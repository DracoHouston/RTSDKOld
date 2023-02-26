// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKUnitComponent.h"
#include "MassEntityView.h"
#include "MassEntitySubsystem.h"
#include "RTSGameSimSubsystem.h"
#include "RTSBatchedScriptCommand.h"
#include "RTSDKScriptExecutionContext.h"
#include "RTSDKFragments.h"

void URTSUnitRegistrationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddFragment<FRTSUnitIDFragment>();
	BuildContext.AddFragment<FRTSUnitComponentFragment>();
	BuildContext.GetMutableObjectFragmentInitializers().Add([=](UObject& Owner, FMassEntityView& EntityView, const EMassTranslationDirection CurrentDirection)
	{
		AActor* actorowner = Cast<AActor>(&Owner);
		if (actorowner != nullptr)
		{
			URTSDKUnitComponent* unit = actorowner->FindComponentByClass<URTSDKUnitComponent>();
			if (unit != nullptr)
			{
				UWorld* world = actorowner->GetWorld();
				if (world != nullptr)
				{
					URTSGameSimSubsystem* sim = world->GetSubsystem<URTSGameSimSubsystem>();
					if (sim != nullptr)
					{
						FRTSUnitIDFragment& unitid = EntityView.GetFragmentData<FRTSUnitIDFragment>();
						unitid.UnitID = sim->RegisterUnit(actorowner, unit, EntityView.GetEntity());
						unit->UnitID = unitid.UnitID;
						FRTSUnitComponentFragment& unitcomp = EntityView.GetFragmentData<FRTSUnitComponentFragment>();
						unitcomp.UnitComponent = unit;
					}
				}
			}
		}
	});
}

void URTSDKUnitComponent::OnRegister()
{
	if (IsRunningCommandlet() || IsRunningCookCommandlet() || GIsCookerLoadingPackage)
	{
		// ignore, we're not doing any registration while cooking or running a commandlet
		Super::OnRegister();
		return;
	}

	if (GetOuter() == nullptr || GetOuter()->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || HasAnyFlags(RF_ArchetypeObject))
	{
		// we won't try registering a CDO's component with Mass
		ensure(false && "temp, wanna know this happened");
		Super::OnRegister();
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr
#if WITH_EDITOR
		|| World->IsPreviewWorld() || (bAutoRegisterInEditorMode == false && World->IsGameWorld() == false)
#endif // WITH_EDITOR
		)
	{
		// we don't care about preview worlds. Those are transient, temporary worlds like the one created when opening a BP editor.
		Super::OnRegister();
		return;
	}

	TConstArrayView<UMassEntityTraitBase*> traits = EntityConfig.GetTraits();
	bool traitexists = false;
	for (int32 i = 0; i < traits.Num(); i++)
	{
		if ((traits[i] != nullptr) && (traits[i]->GetClass() == URTSUnitRegistrationTrait::StaticClass()))
		{
			traitexists = true;
			break;
		}
	}

	if (!traitexists)
	{
		URTSUnitRegistrationTrait* unitregotrait = NewObject<URTSUnitRegistrationTrait>(this);
		EntityConfig.AddTrait(*unitregotrait);
	}
	
	Super::OnRegister();

	URTSGameSimSubsystem* sim = World->GetSubsystem<URTSGameSimSubsystem>();
	if (sim == nullptr)
	{
		return;
	}
	OwningSim = sim;
}

int64 URTSDKUnitComponent::GetUnitID() const
{
	return UnitID;
}

FVector URTSDKUnitComponent::GetUnitInput()
{
	FMassEntityView ent(GetWorld()->GetSubsystem<UMassEntitySubsystem>()->GetEntityManager(), GetEntityHandle());
	FRTSMovementInputFragment& input = ent.GetFragmentData<FRTSMovementInputFragment>();
	return input.Input;
}

void URTSDKUnitComponent::SetUnitInput(FRTSDKScriptExecutionContext inContext, FVector inDir)
{
	if (OwningSim->IsScriptCallingMode())
	{
		inContext.ScriptCommandBuffer->PushCommand<FRTSSetUnitInputScriptCommand>(GetEntityHandle(), (FRTSVector64)inDir);
	}
}
