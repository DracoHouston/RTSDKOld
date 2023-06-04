// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSDKGameSettingRegistry.h"

#include "Engine/LocalPlayer.h"
#include "GameSettingCollection.h"
#include "HAL/Platform.h"
#include "RTSDKGameUserSettings.h"
#include "GameFramework/GameUserSettings.h"
#include "RTSDKLocalPlayer.h"
#include "Templates/Casts.h"
#include "RTSDKPlayerSettingsSubsystem.h"
#include "RTSDKDeveloperSettings.h"
#include "RTSDKGameSettingKeybind.h"
#include "RTSDKKeyBindMetadata.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RTSDKGameSettingRegistry)

DEFINE_LOG_CATEGORY(LogRTSDKGameSettingRegistry);

#define LOCTEXT_NAMESPACE "RTSDK"

URTSDKGameSettingRegistry::URTSDKGameSettingRegistry()
{
}

bool URTSDKGameSettingRegistry::IsFinishedInitializing() const
{
	if (Super::IsFinishedInitializing())
	{
		if (PlayerSettingsSubsystem != nullptr)
		{
			if (PlayerSettingsSubsystem->GetSharedSettings() == nullptr)
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

UGameSettingCollection* URTSDKGameSettingRegistry::InitializeInputSettings(ULocalPlayer* InLocalPlayer)
{
	UGameSettingCollection* Screen = NewObject<UGameSettingCollection>();
	Screen->SetDevName(TEXT("InputSettingsCollection"));
	Screen->SetDisplayName(LOCTEXT("InputSettingsCollection_Name", "Input Settings"));
	Screen->Initialize(InLocalPlayer);
	URTSDKSharedUserSettingsBase* sharedsettings = PlayerSettingsSubsystem->GetSharedSettings();
	URTSDKDeveloperSettings* devsettings = GetMutableDefault<URTSDKDeveloperSettings>();
	for (int32 c = 0; c < devsettings->DefaultInputProfile.PlayerBindContexts.Num(); c++)
	{
		FRTSDKSavedBindContext activebindcontext = sharedsettings->InputProfiles[sharedsettings->ActiveProfile].GetContextByName(
			devsettings->DefaultInputProfile.PlayerBindContexts[c].ContextName);
		UGameSettingCollection* BindContext = NewObject<UGameSettingCollection>();
		BindContext->SetDevName(devsettings->DefaultInputProfile.PlayerBindContexts[c].ContextName);
		BindContext->SetDisplayName(devsettings->DefaultInputProfile.PlayerBindContexts[c].DisplayName);
		BindContext->Initialize(InLocalPlayer);
		for (int32 b = 0; b < devsettings->DefaultInputProfile.PlayerBindContexts[c].PlayerBinds.Num(); b++)
		{
			URTSDKGameSettingKeybind* bind = NewObject<URTSDKGameSettingKeybind>();
			FRTSDKSavedBindPair activebind = activebindcontext.GetBindsByName(devsettings->DefaultInputProfile.PlayerBindContexts[c].PlayerBinds[b].BindName);
			bind->InitializeDefaults(
				devsettings->DefaultInputProfile.PlayerBindContexts[c].PlayerBinds[b], 
				devsettings->DefaultInputProfile.PlayerBindContexts[c].PlayerBinds[b].BindMetadata.Get());
			bind->InitializeBinds(activebind);
			bind->SetDevName(devsettings->DefaultInputProfile.PlayerBindContexts[c].PlayerBinds[b].BindName);
			bind->SetDisplayName(devsettings->DefaultInputProfile.PlayerBindContexts[c].PlayerBinds[b].DisplayName);
			bind->Initialize(InLocalPlayer);
			BindContext->AddSetting(bind);
		}
		Screen->AddSetting(BindContext);
	}
	return Screen;
}

void URTSDKGameSettingRegistry::OnInitialize(ULocalPlayer* InLocalPlayer)
{
	PlayerSettingsSubsystem = InLocalPlayer->GetSubsystem<URTSDKPlayerSettingsSubsystem>();
	InputSettings = InitializeInputSettings(InLocalPlayer);
	RegisterSetting(InputSettings);
}

void URTSDKGameSettingRegistry::SaveChanges()
{
	Super::SaveChanges();
	
	if (PlayerSettingsSubsystem != nullptr)
	{
		// Game user settings need to be applied to handle things like resolution, this saves indirectly
		PlayerSettingsSubsystem->GetLocalSettings()->ApplySettings(false);
		
		PlayerSettingsSubsystem->GetSharedSettings()->ApplySettings();
		PlayerSettingsSubsystem->GetSharedSettings()->SaveSettings();
	}
}

#undef LOCTEXT_NAMESPACE

