// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "RTSDKModManager.generated.h"

class URTSDKGameModDefinition;
class URTSDKFactionModDefinition;
class URTSDKMapModDefinition;
class URTSDKMutatorDefinition;
class URTSDKConfigurableHUDDefinition;
class URTSDKConfigurableInputMappingDefinition;

/**
 * This engine subsystem is responsible for managing the RTSDK Mod feature plugin system
 * RTSDK Mods come in 3 main types, each is meant to be within a game feature plugin.
 * Game Mod - Logic and content common to an implementation of a game, 
 * a set of rules and mechanics that drive the game above what is available in the RTSDK plugins and your game module.
 * Faction Mod - Logic and content specific to 'factions' of units, for a Game Mod
 * Map Mod - Logic and content specific to 'maps', levels/worlds that can be played in, for a Game Mod, and optionally, using specific Faction Mods.
 * 
 * This subsystem keeps track of available Mod game feature plugins via the primary data asset types blueprinted by the feature plugins.
 * RTSDKMapModDefinition populates the map list
 * RTSDKGameModDefinition populates the games list
 * RTSDKFactionModDefintion populates the factions list
 * These can then be used to reach information on the maps, factions and games contained within.
 * 
 * Maps can be thought of as an entry, loading into the associated level and activating as a feature plugin. 
 * the game it depends on, and the factions in play are loaded and activated as game features, and any regular game features any of these depend on.
 * All regular plugins and game modules are already loaded at this point. Game features are hotloaded and unloaded on demand at runtime. 
 * Anything required permanently at runtime should go in a regular plugin or game module. All else should go in Mods.
 */
UCLASS()
class RTSDK_API URTSDKModManager : public UEngineSubsystem
{
	GENERATED_BODY()
	
public:

	/**
	* Refreshes the database after registering or deregistering a mod, and after initial built in game feature load
	*/
	UFUNCTION()
		void BuildMods();

	/**
	* Generic game mod registration, caller is responsible for filling out the mod def
	*/
	UFUNCTION()
		void RegisterGameMod(URTSDKGameModDefinition* inModDef);

	/**
	* Generic faction mod registration, caller is responsible for filling out the mod def
	*/
	UFUNCTION()
		void RegisterFactionMod(URTSDKFactionModDefinition* inModDef);

	/**
	* Generic map mod registration, caller is responsible for filling out the mod def
	*/
	UFUNCTION()
		void RegisterMapMod(URTSDKMapModDefinition* inModDef);

	/**
	* Generic configurable HUD registration, caller is responsible for filling out the HUD def
	*/
	UFUNCTION()
		void RegisterConfigurableHUD(URTSDKConfigurableHUDDefinition* inHUDDef);

	/**
	* Generic configurable Input Mapping registration, caller is responsible for filling out the mapping def
	*/
	UFUNCTION()
		void RegisterConfigurableInputMapping(URTSDKConfigurableInputMappingDefinition* inMappingDef);

	/**
	* Gets a game mod def by its dev name, if it exists. Will return nullptr if it does not.
	*/
	UFUNCTION()
		URTSDKGameModDefinition* GetGameModByName(FName inModDevName) const;

	/**
	* Gets a faction mod def by its dev name, if it exists. Will return nullptr if it does not.
	*/
	UFUNCTION()
		URTSDKFactionModDefinition* GetFactionModByName(FName inModDevName) const;

	/**
	* Gets a map mod def by its dev name, if it exists. Will return nullptr if it does not.
	*/
	UFUNCTION()
		URTSDKMapModDefinition* GetMapModByName(FName inModDevName) const;

	/**
	* Gets a configurable HUD def by its dev name, if it exists. Will return nullptr if it does not.
	*/
	UFUNCTION()
		URTSDKConfigurableHUDDefinition* GetConfigurableHUDByName(FName inHUDDevName) const;

	/**
	* Gets a configurable input mapping def by its dev name, if it exists. Will return nullptr if it does not.
	*/
	UFUNCTION()
		URTSDKConfigurableInputMappingDefinition* GetConfigurableInputMappingByName(FName inMappingDevName) const;

	/**
	* Gets a game mod by its dev name, if it exists. Will return nullptr if it does not.
	*/
	UFUNCTION()
		TArray<URTSDKFactionModDefinition*> GetFactionModsByGameMod(URTSDKGameModDefinition* inGameMod) const;
	
	UFUNCTION()
		TArray<URTSDKMapModDefinition*> GetMapModsByGameMod(URTSDKGameModDefinition* inGameMod) const;

	UFUNCTION()
		TArray<URTSDKConfigurableHUDDefinition*> GetConfigurableHUDsByGameMod(URTSDKGameModDefinition* inGameMod) const;

	UFUNCTION()
		TArray<URTSDKConfigurableInputMappingDefinition*> GetConfigurableMappingsByGameMod(URTSDKGameModDefinition* inGameMod) const;

	UFUNCTION()
		TArray<FString> GetAllGameFeaturesForMods(URTSDKGameModDefinition* inGameMod, TArray<URTSDKFactionModDefinition*> inFactionMods, URTSDKMapModDefinition* inMapMod, TArray<URTSDKMutatorDefinition*> inMutators) const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
		TArray<FString> GetAllGameFeaturesForModsByName(FName inGameMod, TArray<FName> inFactionMods, FName inMapMod, TArray<FName> inMutators) const;

protected:

	UPROPERTY()
		TMap<FName, URTSDKGameModDefinition*> GameModsByName;

	UPROPERTY()
		TMap<FName, URTSDKFactionModDefinition*> FactionModsByName;

	UPROPERTY()
		TMap<FName, URTSDKMapModDefinition*> MapModsByName;

	UPROPERTY()
		TMap<FName, URTSDKMutatorDefinition*> MutatorsByName;

	UPROPERTY()
		TMap<FName, URTSDKConfigurableHUDDefinition*> ConfigurableHUDByName;

	UPROPERTY()
		TMap<FName, URTSDKConfigurableInputMappingDefinition*> ConfigurableMappingsByName;
};
