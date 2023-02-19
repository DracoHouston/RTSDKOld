// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSDKTraits.h"
#include "RTSDKFragments.h"
#include "MassCommonFragments.h"
#include "MassEntityTemplate.h"
#include "MassEntityTemplateRegistry.h"
#include "MassEntityView.h"
#include "MassEntitySubsystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Actor.h"
#include "MassMovementFragments.h"
#include "Translators/MassCapsuleComponentTranslators.h"
#include "VisualLogger/VisualLogger.h"
#include "RTSVisRootComponent.h"
#include "RTSGameSimSubsystem.h"

namespace FMassAgentTraitsHelper
{
	template<typename T>
	T* AsComponent(UObject& Owner)
	{
		T* Component = nullptr;
		if (AActor* AsActor = Cast<AActor>(&Owner))
		{
			Component = AsActor->FindComponentByClass<T>();
		}
		else
		{
			Component = Cast<T>(&Owner);
		}

		UE_CVLOG_UELOG(Component == nullptr, &Owner, LogMass, Error, TEXT("Trying to extract %s from %s failed")
			, *T::StaticClass()->GetName(), *Owner.GetName());

		return Component;
	}
}

void URTSMovementTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.RequireFragment<FRTSUnitIDFragment>();
	BuildContext.AddFragment<FRTSMovementFragment>();
	BuildContext.AddFragment<FRTSMovementInputFragment>();
	BuildContext.AddFragment<FRTSMovementCoreParamsFragment>();
	BuildContext.AddFragment<FRTSSimRootFragment>();
	BuildContext.AddFragment<FRTSCollisionBoundsFragment>();
	BuildContext.GetMutableObjectFragmentInitializers().Add([=](UObject& Owner, FMassEntityView& EntityView, const EMassTranslationDirection CurrentDirection)
	{
		AActor* actorowner = FRTSTraitHelpers::GetOwnerAsActor(&Owner);
		FRTSVector64 min = FRTSVector64::ZeroVector;
		FRTSVector64 max = FRTSVector64::ZeroVector;
		FRTSVector64 size = FRTSVector64::ZeroVector;
		FRTSVector64 feet = FRTSVector64::ZeroVector;
		if (FRTSTraitHelpers::GetBounds(actorowner, min, max, size, feet))
		{
			FRTSCollisionBoundsFragment& bounds = EntityView.GetFragmentData<FRTSCollisionBoundsFragment>();
			bounds.BoundsMax = max;
			bounds.BoundsMin = min;
			bounds.BoundsSize = size;
			bounds.FeetLocation = feet;
			FRTSSimRootFragment& simroot = EntityView.GetFragmentData<FRTSSimRootFragment>();
			simroot.SimRoot = actorowner->GetRootComponent();
			FRTSMovementInputFragment& input = EntityView.GetFragmentData<FRTSMovementInputFragment>();
			input.Input = (FRTSQuat64)(simroot.SimRoot->GetComponentTransform().GetRotation()) * FRTSVector64::ForwardVector;

			FRTSMovementCoreParamsFragment& movecoreparams = EntityView.GetFragmentData<FRTSMovementCoreParamsFragment>();
#if RTSDK_USE_FIXED_POINT == 0
			movecoreparams.MaxSpeed = (double)MaxSpeed;
			movecoreparams.Acceleration = (double)Acceleration;
			movecoreparams.BrakingFriction = (double)BrakingFriction;
			movecoreparams.AirControl = (double)AirControl;
#else
			movecoreparams.MaxSpeed = MaxSpeed;
			movecoreparams.Acceleration = Acceleration;
			movecoreparams.BrakingFriction = BrakingFriction;
			movecoreparams.AirControl = AirControl;
#endif
			
			FRTSMovementFragment& MovementFragment = EntityView.GetFragmentData<FRTSMovementFragment>();
			FRTSTransform64 roottransform = simroot.SimRoot->GetComponentTransform();
			MovementFragment.CurrentTransform = roottransform;
			MovementFragment.PreviousTransform = roottransform;
			MovementFragment.Velocity = FRTSVector64::ZeroVector;
		}
	});
}

void URTSVisRootInterpolationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.RequireFragment<FRTSMovementFragment>();
	BuildContext.AddFragment<FRTSVisRootFragment>();
	BuildContext.GetMutableObjectFragmentInitializers().Add([=](UObject& Owner, FMassEntityView& EntityView, const EMassTranslationDirection CurrentDirection)
		{
			AActor* actorowner = Cast<AActor>(&Owner);
			if (actorowner != nullptr)
			{
				FRTSVisRootFragment& visroot = EntityView.GetFragmentData<FRTSVisRootFragment>();
				URTSVisRootComponent* visrootcomp = actorowner->FindComponentByClass<URTSVisRootComponent>();
				if (visrootcomp != nullptr)
				{
					visroot.VisRoot = visrootcomp;
				}
			}
		});
}

void URTSUnitRegistrationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddFragment<FRTSUnitIDFragment>();
	BuildContext.GetMutableObjectFragmentInitializers().Add([=](UObject& Owner, FMassEntityView& EntityView, const EMassTranslationDirection CurrentDirection)
	{
		AActor* actorowner = Cast<AActor>(&Owner);
		if (actorowner != nullptr)
		{
			UWorld* world = actorowner->GetWorld();
			if (world != nullptr)
			{
				URTSGameSimSubsystem* sim = world->GetSubsystem<URTSGameSimSubsystem>();
				if (sim != nullptr)
				{
					FRTSUnitIDFragment& unitid = EntityView.GetFragmentData<FRTSUnitIDFragment>();
					unitid.UnitID = sim->RegisterUnit(actorowner, EntityView.GetEntity());
				}
			}
		}
	});
}

AActor* FRTSTraitHelpers::GetOwnerAsActor(UObject* inOwner)
{
	if (inOwner != nullptr)
	{
		return Cast<AActor>(inOwner);
	}
	return nullptr;
}

void GetChildBounds(USceneComponent* inChild, FRTSVector64& outMin, FRTSVector64& outMax)
{
	TArray<USceneComponent*> children = TArray<USceneComponent*>();
	inChild->GetChildrenComponents(false, children);
	//FRTSVector64 origin = (FRTSVector64)inChild->Bounds.Origin;
	FRTSVector64 boxextent = (FRTSVector64)inChild->Bounds.BoxExtent;
	FRTSVector64 min = FRTSVector64::ZeroVector - boxextent;
	FRTSVector64 max = FRTSVector64::ZeroVector + boxextent;
	outMin = FRTSVector64::Min(outMin, min);
	outMax = FRTSVector64::Max(outMax, max);
	for (int32 i = 0; i < children.Num(); i++)
	{
		if (!children[i]->IsA<URTSVisRootComponent>())
		{
			GetChildBounds(children[i], outMin, outMax);
		}
	}
}

bool FRTSTraitHelpers::GetBounds(AActor* inActor, FRTSVector64& outMin, FRTSVector64& outMax, FRTSVector64& outSize, FRTSVector64& outFeetPosition)
{
	if (inActor == nullptr)
	{
		return false;
	}
	USceneComponent* simroot = inActor->GetRootComponent();
	if (simroot == nullptr)
	{
		return false;
	}
	TArray<USceneComponent*> rootchildren = TArray<USceneComponent*>();
	simroot->GetChildrenComponents(false, rootchildren);
	//FRTSVector64 origin = (FRTSVector64)simroot->Bounds.Origin;
	FRTSVector64 boxextent = (FRTSVector64)simroot->Bounds.BoxExtent;
	outMin = FRTSVector64::ZeroVector - boxextent;
	outMax = FRTSVector64::ZeroVector + boxextent;
	for (int32 i = 0; i < rootchildren.Num(); i++)
	{
		if (!rootchildren[i]->IsA<URTSVisRootComponent>())
		{
			GetChildBounds(rootchildren[i], outMin, outMax);
		}
	}
	outSize = outMax - outMin;
	outFeetPosition = FRTSVector64::ZeroVector;
	outFeetPosition.Z = outMin.Z;
	return true;
}
