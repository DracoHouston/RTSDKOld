// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassAgentComponent.h"
#include "MassEntityTraitBase.h"
#include "RTSDKScriptExecutionContext.h"
#include "RTSDKUnitComponent.generated.h"

class URTSGameSimSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRTSDKOnUnitCollided, FRTSDKScriptExecutionContext, ScriptContext, FHitResult, TriggeringHit);

/**
*
*/
UCLASS()
class URTSUnitRegistrationTrait : public UMassEntityTraitBase
{
public:
	GENERATED_BODY()

protected:
	//Where the magic happens
	virtual void BuildTemplate(
		FMassEntityTemplateBuildContext& BuildContext,
		const UWorld& World) const override;
};

/**
 * Component that provides script functionality and bridging Mass Entity and the Actor framework.
 * Also registers, as a unit, this component, this component's owning actor and the associated entity
 * in Mass with the game sim subsystem.
 * 
 * Script functionality comes in 3 flavours:
 * Events the sim may trigger at the end of a frame.
 * Accessors that can pull values out of the Mass Entity database.
 * Mirrors of some infrequent data pertinent to player inputs and scripts
 * 
 * This component is often looked for in checks performed during player input,
 * as well as being the target of many queued input commands, which are executed
 * before all other processors on their target frame.
 * 
 * Additionally, this component is the target for many batched script commands,
 * which are executed after all scripts for a frame have triggered.
 */
UCLASS(Blueprintable, ClassGroup = RTSDK, meta = (BlueprintSpawnableComponent), hidecategories = (Sockets, Collision))
class RTSDK_API URTSDKUnitComponent : public UMassAgentComponent
{
	GENERATED_BODY()

public:
	virtual void OnRegister() override;

	/**
	* Gets the UnitID for this unit
	*/
	UFUNCTION(BlueprintPure)
	int64 GetUnitID() const;

	/**
	* UnitID for this unit
	*/
	UPROPERTY()
	int64 UnitID;

	UPROPERTY(BlueprintAssignable)
	FRTSDKOnUnitCollided OnUnitCollided;

	UFUNCTION(BlueprintPure)
		FVector GetUnitInput();

	UFUNCTION(BlueprintCallable)
		void SetUnitInput(FRTSDKScriptExecutionContext inContext, FVector inDir);

	TObjectPtr<URTSGameSimSubsystem> OwningSim;
};
