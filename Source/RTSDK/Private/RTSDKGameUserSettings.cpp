// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKGameUserSettings.h"
#include "RTSDKLocalPlayer.h"
#include "RTSDKDeveloperSettings.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/Gameplaystatics.h"


//
//URTSDKDeveloperSettings* rtsdksettings = GetMutableDefault<URTSDKDeveloperSettings>();
//InputProfiles.Insert(rtsdksettings->DefaultInputProfile, 0);

URTSDKSharedUserSettingsBase::URTSDKSharedUserSettingsBase(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	URTSDKDeveloperSettings* rtsdksettings = GetMutableDefault<URTSDKDeveloperSettings>();
	InputProfiles.Insert(rtsdksettings->DefaultInputProfile, 0);
	ActiveProfile = 0;
}

void URTSDKSharedUserSettingsBase::Initialize(ULocalPlayer* LocalPlayer)
{
	check(LocalPlayer);

	OwningPlayer = LocalPlayer;
}

void URTSDKSharedUserSettings::SaveSettings()
{
	check(OwningPlayer);
	UGameplayStatics::SaveGameToSlot(this, RTSDK::SharedUserSettingsSaveGameSlotName, OwningPlayer->GetLocalPlayerIndex());
}

void URTSDKSharedUserSettings::ApplySettings()
{

}
