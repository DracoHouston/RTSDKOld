// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKGameSettingKeybind.h"

#define LOCTEXT_NAMESPACE "RTSDK"

void URTSDKGameSettingKeybind::StoreInitial()
{
	InitialBinds = Binds;
}

void URTSDKGameSettingKeybind::ResetToDefault()
{
	Binds = DefaultBinds;
}

void URTSDKGameSettingKeybind::RestoreToInitial()
{
	Binds = InitialBinds;
}

void URTSDKGameSettingKeybind::OnInitialized()
{
}

void URTSDKGameSettingKeybind::InitializeDefaults(const FRTSDKSavedBindPair& inDefaultBinds, URTSDKKeyBindMetadata* inMetadata)
{
	DefaultBinds = inDefaultBinds;
	KeybindMetadata = inMetadata;
}

FText URTSDKGameSettingKeybind::GetBindDisplayName() const
{
	return Binds.DisplayName;
}

FText URTSDKGameSettingKeybind::GetBindTooltip() const
{
	return Binds.TooltipText;
}

FText URTSDKGameSettingKeybind::GetBoundKeysDisplayText(bool bPrimaryBind) const
{
	TArray<FText> displaynames;
	if (bPrimaryBind)
	{
		if (Binds.Primary.BoundKeys.Num() > 0)
		{
			for (int32 i = 0; i < Binds.Primary.BoundKeys.Num(); i++)
			{
				displaynames.Add(Binds.Primary.BoundKeys[i].GetDisplayName());
			}
			return FText::Join(FText::FromString(TEXT(", ")), displaynames);
		}
		return LOCTEXT("InputSettingsNoBind", "Not Bound");
	}
	else
	{
		if (Binds.Secondary.BoundKeys.Num() > 0)
		{
			for (int32 i = 0; i < Binds.Secondary.BoundKeys.Num(); i++)
			{
				displaynames.Add(Binds.Secondary.BoundKeys[i].GetDisplayName());
			}
			return FText::Join(FText::FromString(TEXT(", ")), displaynames);
		}
		return LOCTEXT("InputSettingsNoBind", "Not Bound");
	}
}

TArray<FKey> URTSDKGameSettingKeybind::GetBoundKeys(bool bPrimaryBind) const
{
	return bPrimaryBind ? Binds.Primary.BoundKeys : Binds.Secondary.BoundKeys;
}

void URTSDKGameSettingKeybind::SetBoundKeys(const TArray<FKey>& inKeys, bool bPrimaryBind)
{
	bPrimaryBind ? Binds.Primary.BoundKeys = inKeys : Binds.Secondary.BoundKeys = inKeys;
}

void URTSDKGameSettingKeybind::ClearBoundKeys(bool bPrimaryBind)
{
	bPrimaryBind ? Binds.Primary.BoundKeys.Empty() : Binds.Secondary.BoundKeys.Empty();
}

void URTSDKGameSettingKeybind::InitializeBinds(const FRTSDKSavedBindPair& inBinds)
{
	Binds = inBinds;
}

#undef LOCTEXT_NAMESPACE
