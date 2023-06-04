// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameSettingValue.h"
#include "RTSDKTypes.h"
#include "RTSDKGameSettingKeybind.generated.h"

class URTSDKKeyBindMetadata;

/**
 * 
 */
UCLASS()
class RTSDK_API URTSDKGameSettingKeybind : public UGameSettingValue
{
	GENERATED_BODY()
		
public:
	//UGameSetting
	virtual void StoreInitial() override;
	virtual void ResetToDefault() override;
	virtual void RestoreToInitial() override;
	//~UGameSetting
protected:
	//UGameSettingValue
	virtual void OnInitialized() override;
	//~UGameSettingValue
public:
	FRTSDKSavedBindPair Binds;
	FRTSDKSavedBindPair InitialBinds;
	FRTSDKSavedBindPair DefaultBinds;
	TObjectPtr<URTSDKKeyBindMetadata> KeybindMetadata;
	void InitializeDefaults(const FRTSDKSavedBindPair& inDefaultBinds, URTSDKKeyBindMetadata* inMetadata);
	void InitializeBinds(const FRTSDKSavedBindPair& inBinds);

	UFUNCTION(BlueprintCallable)
	void SetBoundKeys(const TArray<FKey>& inKeys, bool bPrimaryBind = true);

	UFUNCTION(BlueprintCallable)
		void ClearBoundKeys(bool bPrimaryBind = true);

	UFUNCTION(BlueprintPure)
	FText GetBindDisplayName() const;

	UFUNCTION(BlueprintPure)
	FText GetBindTooltip() const;

	UFUNCTION(BlueprintPure)
	FText GetBoundKeysDisplayText(bool bPrimaryBind = true) const;

	UFUNCTION(BlueprintPure)
	TArray<FKey> GetBoundKeys(bool bPrimaryBind = true) const;
};
