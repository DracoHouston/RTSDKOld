// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKFeatureAction_AddHUDToSettings.h"
#include "RTSDKModManager.h"
#include "RTSDKHUDLayoutData.h"
#include "RTSDKConfigurableHUDDefinition.h"

void URTSDKFeatureAction_AddHUDToSettings::OnGameFeatureRegistering()
{
	URTSDKModManager* modmanager = GEngine->GetEngineSubsystem<URTSDKModManager>();
	
	URTSDKConfigurableHUDDefinition* newhud = NewObject<URTSDKConfigurableHUDDefinition>(modmanager);
	newhud->Init(
		HUDDevName, 
		HUDDisplayName, 
		HUDDescriptionText, 
		AssociatedGameModDevName, 
		ParentHUDDevName,
		HUDLayoutData->WidgetClass, 
		HUDLayoutData->Elements);
	modmanager->RegisterConfigurableHUD(newhud);
}

void URTSDKFeatureAction_AddHUDToSettings::OnGameFeatureUnregistering()
{
}
