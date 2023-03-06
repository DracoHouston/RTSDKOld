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
#include "RTSDKVisRootComponent.h"
#include "RTSDKGameSimSubsystem.h"

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

void URTSComplexWalkingMovementTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.RequireFragment<FRTSUnitIDFragment>();
	BuildContext.AddFragment<FRTSCurrentLocationFragment>();
	BuildContext.AddFragment<FRTSCurrentRotationFragment>();
	BuildContext.AddFragment<FRTSCurrentScaleFragment>();
	BuildContext.AddFragment<FRTSVelocityFragment>();
	BuildContext.AddFragment<FRTSAngularVelocityFragment>();
	BuildContext.AddFragment<FRTSMovementInputFragment>();
	BuildContext.AddFragment<FRTSLookInputFragment>();
	BuildContext.AddFragment<FRTSAirControlFactorFragment>();
	BuildContext.AddFragment<FRTSMaxVelocityFragment>();
	BuildContext.AddFragment<FRTSMaxAngularVelocityFragment>();
	BuildContext.AddFragment<FRTSMaxAccelerationFragment>();
	BuildContext.AddFragment<FRTSMaxAngularAccelerationFragment>();
	BuildContext.AddFragment<FRTSMaxDecelerationFragment>();
	BuildContext.AddFragment<FRTSMaxAngularDecelerationFragment>();
	BuildContext.AddFragment<FRTSMovementComplexWalkingParamsFragment>();
	BuildContext.AddFragment<FRTSPhysicsParamsFragment>();
	BuildContext.AddFragment<FRTSSimRootFragment>();
	BuildContext.AddFragment<FRTSCollisionBoundsFragment>();
	BuildContext.GetMutableObjectFragmentInitializers().Add([=](UObject& Owner, FMassEntityView& EntityView, const EMassTranslationDirection CurrentDirection)
	{
		AActor* actorowner = FRTSTraitHelpers::GetOwnerAsActor(&Owner);
		FRTSVector64 min = FRTSVector64::ZeroVector;
		FRTSVector64 max = FRTSVector64::ZeroVector;
		FRTSVector64 size = FRTSVector64::ZeroVector;
		FRTSVector64 feet = FRTSVector64::ZeroVector;
		FRTSNumber64 radius = 0.0;
		FRTSNumber64 halfheight = 0.0;
		URTSDKGameSimSubsystem* sim = UWorld::GetSubsystem<URTSDKGameSimSubsystem>(Owner.GetWorld());
		if (FRTSTraitHelpers::GetBounds(sim, actorowner, min, max, size, halfheight, radius, feet))
		{
			FRTSCollisionBoundsFragment& bounds = EntityView.GetFragmentData<FRTSCollisionBoundsFragment>();
			bounds.BoundsMax = max;
			bounds.BoundsMin = min;
			bounds.BoundsSize = size;
			bounds.BoundsHalfHeight = halfheight;
			bounds.BoundsRadius = radius;
			bounds.FeetLocation = feet;
			FRTSSimRootFragment& simroot = EntityView.GetFragmentData<FRTSSimRootFragment>();
			simroot.SimRoot = actorowner->GetRootComponent();
			FRTSMovementInputFragment& moveinput = EntityView.GetFragmentData<FRTSMovementInputFragment>();
			FRTSLookInputFragment& lookinput = EntityView.GetFragmentData<FRTSLookInputFragment>();
			moveinput.Input = (FRTSQuat64)(simroot.SimRoot->GetComponentTransform().GetRotation()) * FRTSVector64::ForwardVector;
			lookinput.Input = moveinput.Input.ToOrientationRotator();
			
			FRTSAirControlFactorFragment& aircontrol = EntityView.GetFragmentData<FRTSAirControlFactorFragment>();
			FRTSMaxVelocityFragment& maxvelocity = EntityView.GetFragmentData<FRTSMaxVelocityFragment>();
			FRTSMaxAngularVelocityFragment& maxangularvelocity = EntityView.GetFragmentData<FRTSMaxAngularVelocityFragment>();
			FRTSMaxAccelerationFragment& maxacceleration = EntityView.GetFragmentData<FRTSMaxAccelerationFragment>();
			FRTSMaxAngularAccelerationFragment& maxangularacceleration = EntityView.GetFragmentData<FRTSMaxAngularAccelerationFragment>();
			FRTSMaxDecelerationFragment& maxdeceleration = EntityView.GetFragmentData<FRTSMaxDecelerationFragment>();
			FRTSMaxAngularDecelerationFragment& maxangulardeceleration = EntityView.GetFragmentData<FRTSMaxAngularDecelerationFragment>();
			
			FRTSPhysicsParamsFragment& physicsparams = EntityView.GetFragmentData<FRTSPhysicsParamsFragment>();
			FRTSMovementComplexWalkingParamsFragment& movecomplexparams = EntityView.GetFragmentData<FRTSMovementComplexWalkingParamsFragment>();

			FRTSNumber64 timestep = 0.0;
			FRTSNumber64 uutometers = 100.0;
			if (sim != nullptr)
			{
				timestep = sim->GetTimestep();
				uutometers = sim->GetMetersToUUScale();
			}
#if RTSDK_USE_FIXED_POINT == 0
			maxvelocity.Velocity = ((FRTSNumber64)((double)MaxVelocity) * uutometers) * timestep;
			maxacceleration.Acceleration = ((FRTSNumber64)((double)Acceleration) * uutometers) * timestep;
			maxdeceleration.Deceleration = ((FRTSNumber64)((double)Deceleration) * uutometers) * timestep;
			FRTSNumber64 angaccpitch = (FRTSNumber64)((double)MaxAngularAccelerationPitch) * timestep;
			FRTSNumber64 angaccyaw = (FRTSNumber64)((double)MaxAngularAccelerationYaw) * timestep;
			FRTSNumber64 angaccroll = (FRTSNumber64)((double)MaxAngularAccelerationRoll) * timestep;
			FRTSNumber64 angdecpitch = (FRTSNumber64)((double)MaxAngularDecelerationPitch) * timestep;
			FRTSNumber64 angdecyaw = (FRTSNumber64)((double)MaxAngularDecelerationYaw) * timestep;
			FRTSNumber64 angdecroll = (FRTSNumber64)((double)MaxAngularDecelerationRoll) * timestep;
			FRTSNumber64 angvelpitch = (FRTSNumber64)((double)MaxAngularVelocityPitch) * timestep;
			FRTSNumber64 angvelyaw = (FRTSNumber64)((double)MaxAngularVelocityYaw) * timestep;
			FRTSNumber64 angvelroll = (FRTSNumber64)((double)MaxAngularVelocityRoll) * timestep;
			maxangularacceleration.AngularAcceleration = FRTSRotator64(angaccpitch, angaccyaw, angaccroll);
			maxangulardeceleration.AngularDeceleration = FRTSRotator64(angdecpitch, angdecyaw, angdecroll);
			maxangularvelocity.AngularVelocity = FRTSRotator64(angvelpitch, angvelyaw, angvelroll);
			aircontrol.AirControl = (double)AirControl;

			physicsparams.Mass = (double)Mass;
			physicsparams.Volume = (double)Volume;
			physicsparams.Density = FRTSMath::IsNearlyZero(physicsparams.Mass) || FRTSMath::IsNearlyZero(physicsparams.Volume) ? 0.0 : physicsparams.Mass / physicsparams.Volume;

			movecomplexparams.MaxWalkableAngle = FRTSMath::Cos(FRTSMath::DegreesToRadians((FRTSNumber64)((double)MaxWalkableAngle)));
			movecomplexparams.StepUpHeight = (double)MaxStepUpHeight;
			movecomplexparams.StepDownHeight = (double)MaxStepDownHeight;
#else
			maxvelocity.Velocity = (MaxVelocity * uutometers) * timestep;
			maxacceleration.Acceleration = (Acceleration * uutometers) * timestep;
			maxdeceleration.Deceleration = (Deceleration * uutometers) * timestep;
			FRTSNumber64 angaccpitch = MaxAngularAccelerationPitch * timestep;
			FRTSNumber64 angaccyaw = MaxAngularAccelerationYaw * timestep;
			FRTSNumber64 angaccroll = MaxAngularAccelerationRoll * timestep;
			FRTSNumber64 angdecpitch = MaxAngularDecelerationPitch * timestep;
			FRTSNumber64 angdecyaw = MaxAngularDecelerationYaw * timestep;
			FRTSNumber64 angdecroll = MaxAngularDecelerationRoll * timestep;
			FRTSNumber64 angvelpitch = MaxAngularVelocityPitch * timestep;
			FRTSNumber64 angvelyaw = MaxAngularVelocityYaw * timestep;
			FRTSNumber64 angvelroll = MaxAngularVelocityRoll * timestep;
			maxangularacceleration.AngularAcceleration = FRTSRotator64(angaccpitch, angaccyaw, angaccroll);
			maxangulardeceleration.AngularDeceleration = FRTSRotator64(angdecpitch, angdecyaw, angdecroll);
			maxangularvelocity.AngularVelocity = FRTSRotator64(angvelpitch, angvelyaw, angvelroll);
			aircontrol.AirControl = AirControl;

			physicsparams.Mass = Mass;
			physicsparams.Volume = Volume;
			physicsparams.Density = FRTSMath::IsNearlyZero(physicsparams.Mass) || FRTSMath::IsNearlyZero(physicsparams.Volume) ? 0.0 : physicsparams.Mass / physicsparams.Volume;

			movecomplexparams.MaxWalkableAngle = FRTSMath::Cos(FRTSMath::DegreesToRadians(MaxWalkableAngle));
			movecomplexparams.StepUpHeight = MaxStepUpHeight;
			movecomplexparams.StepDownHeight = MaxStepDownHeight;
#endif
			
			FRTSCurrentLocationFragment& LocFragment = EntityView.GetFragmentData<FRTSCurrentLocationFragment>();
			FRTSCurrentRotationFragment& RotFragment = EntityView.GetFragmentData<FRTSCurrentRotationFragment>();
			FRTSCurrentScaleFragment& ScaleFragment = EntityView.GetFragmentData<FRTSCurrentScaleFragment>();
			FRTSVelocityFragment& VelFragment = EntityView.GetFragmentData<FRTSVelocityFragment>();
			FRTSAngularVelocityFragment& AngVelFragment = EntityView.GetFragmentData<FRTSAngularVelocityFragment>();
			FRTSTransform64 roottransform = simroot.SimRoot->GetComponentTransform();
			ScaleFragment.Scale = roottransform.GetScale3D();
			LocFragment.Location = roottransform.GetLocation();
			RotFragment.Rotation = simroot.SimRoot->GetRelativeRotation();
			VelFragment.Velocity = FRTSVector64::ZeroVector;
			AngVelFragment.AngularVelocity = FRTSRotator64::ZeroRotator;
			
		}
	});
}

void URTSVisRootInterpolationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.RequireFragment<FRTSUnitIDFragment>();
	BuildContext.RequireFragment<FRTSCurrentLocationFragment>();
	BuildContext.RequireFragment<FRTSCurrentRotationFragment>();
	BuildContext.RequireFragment<FRTSCurrentScaleFragment>();
	BuildContext.RequireFragment<FRTSSimRootFragment>();
	BuildContext.AddFragment<FRTSVisRootFragment>();
	BuildContext.AddFragment<FRTSPreviousLocationFragment>();
	BuildContext.AddFragment<FRTSPreviousRotationFragment>();
	BuildContext.AddFragment<FRTSPreviousScaleFragment>();
	BuildContext.GetMutableObjectFragmentInitializers().Add([=](UObject& Owner, FMassEntityView& EntityView, const EMassTranslationDirection CurrentDirection)
	{
		AActor* actorowner = Cast<AActor>(&Owner);
		if (actorowner != nullptr)
		{
			FRTSVisRootFragment& visroot = EntityView.GetFragmentData<FRTSVisRootFragment>();
			URTSDKVisRootComponent* visrootcomp = actorowner->FindComponentByClass<URTSDKVisRootComponent>();
			if (visrootcomp != nullptr)
			{
				visroot.VisRoot = visrootcomp;
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
		if (!children[i]->IsA<URTSDKVisRootComponent>())
		{
			GetChildBounds(children[i], outMin, outMax);
		}
	}
}

bool FRTSTraitHelpers::GetBounds(URTSDKGameSimSubsystem* inSim, AActor* inActor, FRTSVector64& outMin, FRTSVector64& outMax, FRTSVector64& outSize, FRTSNumber64& outHalfHeight, FRTSNumber64& outRadius, FRTSVector64& outFeetPosition )
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
	FRTSVector64 globalgravdir = FRTSVector64::DownVector;
	if (inSim != nullptr)
	{
		globalgravdir = inSim->GetGravityDirection();
	}

	TArray<USceneComponent*> rootchildren = TArray<USceneComponent*>();
	simroot->GetChildrenComponents(false, rootchildren);
	//FRTSVector64 origin = (FRTSVector64)simroot->Bounds.Origin;
	FRTSVector64 boxextent = (FRTSVector64)simroot->Bounds.BoxExtent;
	outMin = FRTSVector64::ZeroVector - boxextent;
	outMax = FRTSVector64::ZeroVector + boxextent;
	for (int32 i = 0; i < rootchildren.Num(); i++)
	{
		if (!rootchildren[i]->IsA<URTSDKVisRootComponent>())
		{
			GetChildBounds(rootchildren[i], outMin, outMax);
		}
	}
	outSize = outMax - outMin;
	outHalfHeight = FRTSMath::Abs(outMin.Z);
	outFeetPosition = globalgravdir * outHalfHeight;
	outRadius = FRTSMath::Abs(FRTSMath::Max(outMax.X, outMax.Y));
	return true;
}
