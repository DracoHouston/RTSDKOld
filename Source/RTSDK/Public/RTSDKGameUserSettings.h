// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RTSDKConstants.h"
#include "RTSDKTypes.h"
#include "GameFramework/SaveGame.h"
#include "InputCoreTypes.h"
#include "RTSDKGameUserSettings.generated.h"

class UInputAction;
class UInputMappingContext;
class URTSDKLocalPlayer;



/**
 *
 */
UCLASS(abstract)
class RTSDK_API URTSDKSharedUserSettingsBase : public USaveGame
{
	GENERATED_BODY()
public:

	URTSDKSharedUserSettingsBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere)
		TArray<FRTSDKInputSettingsProfile> InputProfiles;

	UPROPERTY(EditAnywhere)
		int32 ActiveProfile;

	UFUNCTION()
	virtual void Initialize(ULocalPlayer* LocalPlayer);

	UFUNCTION()
	virtual void SaveSettings() {}

	UFUNCTION()
	virtual void ApplySettings() {}

protected:
	UPROPERTY(Transient)
		TObjectPtr<ULocalPlayer> OwningPlayer = nullptr;
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class RTSDK_API URTSDKSharedUserSettings : public URTSDKSharedUserSettingsBase
{
	GENERATED_BODY()
public:	


	virtual void SaveSettings() override;
	virtual void ApplySettings() override;
};