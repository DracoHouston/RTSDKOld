// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RTSDKFactionModDefinition.generated.h"

class URTSDKGameModDefinition;
class URTSDKFactionDefinition;

/**
 * Runtime definition of a Faction Mod, logic and content for a set of units.
 * Associated with a Game Mod, will work with any Game Mod derived from 
 * the designated required Game Mod.
 * Faction Mods may depend on another Faction Mod, allowing for modifications
 * from a base. Eg you might have the base Faction Mod, with a campaign and
 * multiplayer faction mod. The multiplayer faction mod can in turn be used
 * as a base for a user mod that further changes the faction.
 * 
 * Unless faction choice is restricted by a Map Mod, Faction Mods compatible with the
 * active Game Mod can be picked.
 * 
 * Populates the Mod Manager list of Faction Mods.
 */
UCLASS()
class RTSDK_API URTSDKFactionModDefinition : public UObject
{
	GENERATED_BODY()
	
public:

	UFUNCTION()
		void Init(
			FName inDevName, 
			FText inDisplayName, 
			FString inGameFeatureName, 
			TSoftClassPtr<URTSDKFactionDefinition> inFactionClass,
			FName inAssociatedGameModName,
			bool bInIsAbstractMod,
			FName inParentFactionModName);

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
		TSoftClassPtr<URTSDKFactionDefinition> FactionClass;

	UPROPERTY(transient)
		FName ParentFactionModName;

	UPROPERTY(transient)
		TObjectPtr<URTSDKFactionModDefinition> ParentFactionMod;

	UPROPERTY(transient)
		FName AssociatedGameModName;

	UPROPERTY(transient)
		TObjectPtr<URTSDKGameModDefinition> AssociatedGameMod;

	UPROPERTY(transient)
		bool bIsValid;

	UPROPERTY(transient)
		bool bIsAbstractMod;

};
