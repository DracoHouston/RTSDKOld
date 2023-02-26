// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "RTSDKTypes.h"
#include "RTSConstants.h"
#include "FixedPointNumbers.h"
#include "RTSDKTraits.generated.h"

struct RTSDK_API FRTSTraitHelpers
{
	static AActor* GetOwnerAsActor(UObject* inOwner);
	

	static bool GetBounds(AActor* inActor, FRTSVector64& outMin, FRTSVector64& outMax, FRTSVector64& outSize, FRTSNumber64& outHalfHeight, FRTSNumber64& outRadius, FRTSVector64& outFeetPosition);
};

/**
*
*/
UCLASS()
class URTSMovementTrait : public UMassEntityTraitBase
{
public:
	GENERATED_BODY()

		//Max speed, in meters per second
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FFixed64 MaxSpeed;

	//Acceleration, in meters per second squared
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FFixed64 Acceleration;

	//Braking Friction, as deceleration in meters per second squared
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FFixed64 BrakingFriction;

	//Air Control, as a scaling factor, typically between 0.0-1.0
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FFixed64 AirControl;

	//Mass, in kilograms
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FFixed64 Mass;

	//Volume, in meters cubed
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FFixed64 Volume;

	//Maximum height unit may step up onto walkable geometry, in meters
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FFixed64 MaxStepUpHeight;

	//Maximum height unit may step down onto walkable geometry, in meters
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FFixed64 MaxStepDownHeight;

	//Maximum walkable angle unit may walk upon, in degrees
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FFixed64 MaxWalkableAngle;

protected:
	//Where the magic happens
	virtual void BuildTemplate(
		FMassEntityTemplateBuildContext& BuildContext,
		const UWorld& World) const override;
};

/**
*
*/
UCLASS()
class URTSVisRootInterpolationTrait : public UMassEntityTraitBase
{
public:
	GENERATED_BODY()

protected:
	//Where the magic happens
	virtual void BuildTemplate(
		FMassEntityTemplateBuildContext& BuildContext,
		const UWorld& World) const override;
};