// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RTSConstants.h"
#include "MassCommonTypes.h"
#include "RTSGameSimSubsystem.generated.h"

USTRUCT(BlueprintType)
struct RTSDK_API FRTSCommandInputInfo
{
public:
	GENERATED_BODY()

};

USTRUCT(BlueprintType)
struct RTSDK_API FRTSUnitActorAndMassID
{
public:
	GENERATED_BODY()

	FMassEntityHandle UnitHandle;
	TWeakObjectPtr<AActor> UnitActor;
};

/**
 * 
 */
UCLASS()
class RTSDK_API URTSGameSimSubsystem : public UTickableWorldSubsystem
{
public:
	GENERATED_BODY()
	
	//~USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void PostInitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	//~End of USubsystem interface

	bool IsSimRunning()
	{
		return bSimIsRunning;
	}

	int32 GetFrameCount() const
	{
		return FrameCount;
	}

	void SetFrameCount(int32 inFrame)
	{
		FrameCount = inFrame;
	}

	void ResetFrameCount()
	{
		SetFrameCount(0);
	}

	void AdvanceFrame();

	bool ShouldAdvanceFrame() const
	{
		UWorld* world = GetWorld();
		if (world == nullptr)
		{
			return false;
		}
		FRTSNumber64 timeovertimestep = FRTSMath::FloorToDouble((FRTSNumber64)world->TimeSeconds / TimeStep);
		return timeovertimestep > (FRTSNumber64)FrameCount;
	}

	int32 GetFrameDelay() const
	{
		return FrameDelay;
	}

	void SetFrameDelay(int32 inFrameCount)
	{
		FrameDelay = inFrameCount;
	}

	FRTSNumber64 GetTimestep() const
	{
		return TimeStep;
	}

	void SetTimestep(FRTSNumber64 inTimeStep)
	{
		TargetUPS = inTimeStep;
		TimeStep = FRTSNumber64::Make(1.0) / inTimeStep;
	}

	FRTSNumber64 GetMetersToUUScale() const
	{
		return MetersToUUScale;
	}

	void SetMetersToUUScale(FRTSNumber64 inMetersToUUScale)
	{
		MetersToUUScale = inMetersToUUScale;
	}

	FRTSNumber64 GetGravityAcceleration() const
	{
		return GravityAcceleration;
	}

	void SetGravityAcceleration(FRTSNumber64 inGravityAcceleration)
	{
		GravityAcceleration = (inGravityAcceleration * MetersToUUScale) * TimeStep;
		GravityVector = GravityAcceleration > FRTSNumber64::Make(0.0) ? GravityDirection * GravityAcceleration : FRTSVector64::ZeroVector;
	}

	FRTSNumber64 GetTerminalVelocity() const
	{
		return TerminalVelocity;
	}

	void SetTerminalVelocity(FRTSNumber64 inTerminalVelocity)
	{
		TerminalVelocity = (inTerminalVelocity * MetersToUUScale) * TimeStep;
	}

	FRTSVector64 GetGravityDirection() const
	{
		return GravityDirection;
	}

	FRTSVector64 GetGravityVector() const
	{
		return GravityVector;
	}

	void SetGravityDirection(FRTSVector64 inGravityDirection)
	{
		GravityDirection = inGravityDirection;
		GravityVector = GravityAcceleration > FRTSNumber64::Make(0.0) ? GravityDirection * GravityAcceleration : FRTSVector64::ZeroVector;
	}

	TArray<FRTSCommandInputInfo> GetCurrentCommandInputs() const
	{
		return GetCommandInputsByFrame(FrameCount);
	}

	TArray<FRTSCommandInputInfo> GetCommandInputsByFrame(int32 inFrame) const
	{
		const TArray<FRTSCommandInputInfo>* retval = nullptr;
		retval = CommandInputsByFrame.Find(inFrame);
		return *retval;
	}

	void AddCommandInputByFrame(const FRTSCommandInputInfo& inCommandInput, int32 inFrame)
	{
		TArray<FRTSCommandInputInfo>& frameinputs = CommandInputsByFrame.FindOrAdd(inFrame);
		frameinputs.Add(inCommandInput);
	}

	void EnqueueCommandInput(const FRTSCommandInputInfo& inCommandInput)
	{
		AddCommandInputByFrame(inCommandInput, FrameCount + FrameDelay);
	}
	
	int64 RegisterUnit(AActor* inUnitActor, FMassEntityHandle inUnitHandle)
	{
		if ((inUnitActor == nullptr) || (!inUnitHandle.IsValid()))
		{
			return -1;
		}
		
		int64 newid = ClaimUnitID();
		FRTSUnitActorAndMassID& newunit = UnitsByID.Add(newid);
		newunit.UnitActor = inUnitActor;
		newunit.UnitHandle = inUnitHandle;
		return newid;
	}

protected:
	bool bSimIsRunning;
	
	int32 FrameCount;
	int32 TargetUPS;
	FRTSNumber64 TimeStep;

	UPROPERTY(EditAnywhere, Category = Mass)
	TObjectPtr<UMassCompositeProcessor> SimProcessor = nullptr;

	TSharedPtr<FMassEntityManager> EntityManager;

	int32 FrameDelay;
	TMap<int32, TArray<FRTSCommandInputInfo>> CommandInputsByFrame;
	
	TMap<int64, FRTSUnitActorAndMassID> UnitsByID;
	int64 NextUnitID;
	
	FRTSNumber64 MetersToUUScale;
	FRTSNumber64 GravityAcceleration;
	FRTSNumber64 TerminalVelocity;
	FRTSVector64 GravityDirection;
	FRTSVector64 GravityVector;

	void ResetUnits()
	{
		NextUnitID = 0;
		UnitsByID.Empty();
	}
	
	int64 ClaimUnitID()
	{
		int64 retval = NextUnitID;
		NextUnitID++;
		return retval;
	}
};
