// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "RTSDKWorldSettings.generated.h"

USTRUCT()
struct FRTSDKPIECommanderSetupInfo
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(EditAnywhere)
	FText DisplayName;

	UPROPERTY(EditAnywhere)
	FString AdditionalOptions;

	UPROPERTY(EditAnywhere)
	bool bIsBot;

	//Constructor
	FRTSDKPIECommanderSetupInfo()
	{
		DisplayName = FText::GetEmpty();
		AdditionalOptions.Empty();
		bIsBot = false;
	}
};

USTRUCT()
struct FRTSDKPIEForceSetupInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	FText DisplayName;

	UPROPERTY(EditAnywhere)
	FString AdditionalOptions;
	
	UPROPERTY(EditAnywhere)
	TArray<FRTSDKPIECommanderSetupInfo> PIECommanders;

	//Constructor
	FRTSDKPIEForceSetupInfo()
	{
		DisplayName = FText::GetEmpty();
		AdditionalOptions.Empty();
	}
};

USTRUCT()
struct FRTSDKPIETeamSetupInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	FText DisplayName;

	UPROPERTY(EditAnywhere)
	FString AdditionalOptions;

	UPROPERTY(EditAnywhere)
	TArray<FRTSDKPIEForceSetupInfo> PIEForces;

	//Constructor
	FRTSDKPIETeamSetupInfo()
	{
		DisplayName = FText::GetEmpty();
		AdditionalOptions.Empty();
	}
};

/**
 * 
 */
UCLASS()
class RTSDK_API ARTSDKWorldSettings : public AWorldSettings
{
	GENERATED_BODY()
public:
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere)
		TArray<FRTSDKPIETeamSetupInfo> PIEMatchSetup;
#endif
};
