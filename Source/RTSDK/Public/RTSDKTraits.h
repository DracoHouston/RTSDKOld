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
	

	static bool GetBounds(AActor* inActor, FRTSVector64& outMin, FRTSVector64& outMax, FRTSVector64& outSize, FRTSVector64& outFeetPosition);
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