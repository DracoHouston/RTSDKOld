// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RTSDKGameModDefinition.generated.h"

class URTSDKModManager;
class URTSDKMapModDefinition;
class URTSDKFactionModDefinition;
class URTSDKConfigurableHUDDefinition;

/**
 * A definition of a Game Mod, logic and content for an implementation of
 * a game in RTSDK.
 * Map and Faction Mods are dependent on Game Mods.
 * Game Mods can depend on other Game Mods, but only 1 is the active Game at a time.
 * Eg. Campaign Game and Multiplayer Game derived from Base Game
 * And some user Game Mod derived from Multiplayer Game as the actual active game.
 * or for this case: Base->Multiplayer->User
 * Map Mods will accept children of their designated Game Mod, as will Faction Mods.
 * 
 * Populates the Mod Manager list of Game Mods.
 */
UCLASS()
class RTSDK_API URTSDKGameModDefinition : public UObject
{
	GENERATED_BODY()
	
public:

	UFUNCTION()
		void Init(FName inDevName, const FText& inDisplayName, const FString& inGameFeatureName, bool bInIsAbstract, FName inParentModName);

	UFUNCTION()
		void BuildModDependencies(URTSDKModManager* inModManager);

	UFUNCTION()
		void BuildMod(URTSDKModManager* inModManager);

	UPROPERTY(transient)
		FText ModDisplayName;

	UPROPERTY(transient)
		FName ModDevName;

	UPROPERTY(transient)
		FString GameFeatureName;

	UPROPERTY(transient)
		TArray<FString> AdditionalGameFeatureNames;

	UPROPERTY(transient)
		TArray<FString> CombinedGameFeatureURLs;

	UPROPERTY(transient)
		FName ParentGameModName;

	UPROPERTY(transient)
		TObjectPtr<URTSDKGameModDefinition> ParentGameMod;

	UPROPERTY(transient)
		bool bIsValid;

	UPROPERTY(transient)
		bool bIsAbstractMod;

	UPROPERTY(transient)
		TArray<URTSDKMapModDefinition*> ValidMapMods;

	UPROPERTY(transient)
		TArray<URTSDKFactionModDefinition*> ValidFactionMods;

	UPROPERTY(transient)
		TArray<URTSDKConfigurableHUDDefinition*> ValidConfigurableHUDs;
};
