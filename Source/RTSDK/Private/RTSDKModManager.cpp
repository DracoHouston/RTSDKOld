// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKModManager.h"
#include "RTSDKGameModDefinition.h"
#include "RTSDKFactionModDefinition.h"
#include "RTSDKMapModDefinition.h" 
#include "RTSDKMutatorDefinition.h"
#include "RTSDKConfigurableHUDDefinition.h"
#include "RTSDKConfigurableInputMappingDefinition.h"

void URTSDKModManager::BuildMods()
{
	for (auto It = GameModsByName.CreateIterator(); It; ++It)
	{
		It->Value->BuildModDependencies(this);
	}

	for (auto It = FactionModsByName.CreateIterator(); It; ++It)
	{
		It->Value->BuildModDependencies(this);
	}

	for (auto It = MapModsByName.CreateIterator(); It; ++It)
	{
		It->Value->BuildModDependencies(this);
	}

	for (auto It = MutatorsByName.CreateIterator(); It; ++It)
	{
		It->Value->BuildModDependencies(this);
	}

	for (auto It = ConfigurableHUDByName.CreateIterator(); It; ++It)
	{
		It->Value->BuildModDependencies(this);
	}

	for (auto It = FactionModsByName.CreateIterator(); It; ++It)
	{
		It->Value->BuildMod(this);
	}

	for (auto It = MapModsByName.CreateIterator(); It; ++It)
	{
		It->Value->BuildMod(this);
	}

	for (auto It = MutatorsByName.CreateIterator(); It; ++It)
	{
		It->Value->BuildMod(this);
	}

	for (auto It = ConfigurableHUDByName.CreateIterator(); It; ++It)
	{
		It->Value->BuildMod(this);
	}

	for (auto It = GameModsByName.CreateIterator(); It; ++It)
	{
		It->Value->BuildMod(this);
	}
}

void URTSDKModManager::RegisterGameMod(URTSDKGameModDefinition* inModDef)
{
	if (inModDef == nullptr)
	{
		return;
	}
	if (inModDef->ModDevName.IsNone())
	{
		return;
	}
	GameModsByName.Add(inModDef->ModDevName, inModDef);
}

void URTSDKModManager::RegisterFactionMod(URTSDKFactionModDefinition* inModDef)
{
	if (inModDef == nullptr)
	{
		return;
	}
	if (inModDef->ModDevName.IsNone())
	{
		return;
	}
	FactionModsByName.Add(inModDef->ModDevName, inModDef);
}

void URTSDKModManager::RegisterMapMod(URTSDKMapModDefinition* inModDef)
{
	if (inModDef == nullptr)
	{
		return;
	}
	if (inModDef->ModDevName.IsNone())
	{
		return;
	}
	MapModsByName.Add(inModDef->ModDevName, inModDef);
}

void URTSDKModManager::RegisterConfigurableHUD(URTSDKConfigurableHUDDefinition* inHUDDef)
{
	if (inHUDDef == nullptr)
	{
		return;
	}
	if (inHUDDef->HUDDevName.IsNone())
	{
		return;
	}
	ConfigurableHUDByName.Add(inHUDDef->HUDDevName, inHUDDef);
}

void URTSDKModManager::RegisterConfigurableInputMapping(URTSDKConfigurableInputMappingDefinition* inMappingDef)
{
	if (inMappingDef == nullptr)
	{
		return;
	}
	if (inMappingDef->MappingDevName.IsNone())
	{
		return;
	}
	ConfigurableMappingsByName.Add(inMappingDef->MappingDevName, inMappingDef);
}

URTSDKGameModDefinition* URTSDKModManager::GetGameModByName(FName inModDevName) const
{
	if (GameModsByName.Contains(inModDevName))
	{
		return GameModsByName[inModDevName];
	}
	return nullptr;
}

URTSDKFactionModDefinition* URTSDKModManager::GetFactionModByName(FName inModDevName) const
{
	if (FactionModsByName.Contains(inModDevName))
	{
		return FactionModsByName[inModDevName];
	}
	return nullptr;
}

URTSDKMapModDefinition* URTSDKModManager::GetMapModByName(FName inModDevName) const
{
	if (MapModsByName.Contains(inModDevName))
	{
		return MapModsByName[inModDevName];
	}
	return nullptr;
}

URTSDKConfigurableHUDDefinition* URTSDKModManager::GetConfigurableHUDByName(FName inHUDDevName) const
{
	if (ConfigurableHUDByName.Contains(inHUDDevName))
	{
		return ConfigurableHUDByName[inHUDDevName];
	}
	return nullptr;
}

URTSDKConfigurableInputMappingDefinition* URTSDKModManager::GetConfigurableInputMappingByName(FName inMappingDevName) const
{
	if (ConfigurableMappingsByName.Contains(inMappingDevName))
	{
		return ConfigurableMappingsByName[inMappingDevName];
	}
	return nullptr;
}

TArray<URTSDKFactionModDefinition*> URTSDKModManager::GetFactionModsByGameMod(URTSDKGameModDefinition* inGameMod) const
{
	TArray<URTSDKFactionModDefinition*> factions;
	for (auto It = FactionModsByName.CreateIterator(); It; ++It)
	{
		if ((It->Value->bIsValid) && (!It->Value->bIsAbstractMod) && (It->Value->AssociatedGameMod == inGameMod))
		{
			factions.Add(It->Value);
		}
	}
	return factions;
}

TArray<URTSDKMapModDefinition*> URTSDKModManager::GetMapModsByGameMod(URTSDKGameModDefinition* inGameMod) const
{
	TArray<URTSDKMapModDefinition*> maps;
	for (auto It = MapModsByName.CreateIterator(); It; ++It)
	{
		if ((It->Value->bIsValid) && (!It->Value->bIsAbstractMod) && (It->Value->AssociatedGameMod == inGameMod))
		{
			maps.Add(It->Value);
		}
	}
	return maps;
}

TArray<URTSDKConfigurableHUDDefinition*> URTSDKModManager::GetConfigurableHUDsByGameMod(URTSDKGameModDefinition* inGameMod) const
{
	TArray<URTSDKConfigurableHUDDefinition*> huds;
	for (auto It = ConfigurableHUDByName.CreateIterator(); It; ++It)
	{
		if ((It->Value->bIsValid) && (!It->Value->bIsAbstractHUD) && (It->Value->AssociatedGameMod == inGameMod))
		{
			huds.Add(It->Value);
		}
	}
	return huds;
}

TArray<URTSDKConfigurableInputMappingDefinition*> URTSDKModManager::GetConfigurableMappingsByGameMod(URTSDKGameModDefinition* inGameMod) const
{
	TArray<URTSDKConfigurableInputMappingDefinition*> mappings;
	for (auto It = ConfigurableMappingsByName.CreateIterator(); It; ++It)
	{
		if ((It->Value->bIsValid) && (It->Value->AssociatedGameMod == inGameMod))
		{
			mappings.Add(It->Value);
		}
	}
	return mappings;
}

TArray<FString> URTSDKModManager::GetAllGameFeaturesForMods(URTSDKGameModDefinition* inGameMod, TArray<URTSDKFactionModDefinition*> inFactionMods, URTSDKMapModDefinition* inMapMod, TArray<URTSDKMutatorDefinition*> inMutators) const
{
	TArray<FString> retval = inGameMod->CombinedGameFeatureURLs;
	for (int32 i = 0; i < inFactionMods.Num(); i++)
	{
		for (int32 j = 0; j < inFactionMods[i]->CombinedGameFeatureURLs.Num(); j++)
		{
			retval.AddUnique(inFactionMods[i]->CombinedGameFeatureURLs[j]);
		}
	}
	for (int32 i = 0; i < inMapMod->CombinedGameFeatureURLs.Num(); i++)
	{
		retval.AddUnique(inMapMod->CombinedGameFeatureURLs[i]);
	}
	for (int32 i = 0; i < inMutators.Num(); i++)
	{
		for (int32 j = 0; j < inMutators[i]->CombinedGameFeatureURLs.Num(); j++)
		{
			retval.AddUnique(inMutators[i]->CombinedGameFeatureURLs[j]);
		}
	}

	return retval;
}

TArray<FString> URTSDKModManager::GetAllGameFeaturesForModsByName(FName inGameMod, TArray<FName> inFactionMods, FName inMapMod, TArray<FName> inMutators) const
{
	URTSDKGameModDefinition* gamemod = nullptr;
	if (GameModsByName.Contains(inGameMod))
	{
		gamemod = GameModsByName[inGameMod];
	}
	else
	{
		return TArray<FString>();
	}
	TArray<URTSDKFactionModDefinition*> factionmods;
	factionmods.Empty(inFactionMods.Num());
	for (int32 i = 0; i < inFactionMods.Num(); i++)
	{
		if (FactionModsByName.Contains(inFactionMods[i]))
		{
			factionmods.Add(FactionModsByName[inFactionMods[i]]);
		}
		else
		{
			return TArray<FString>();
		}
	}
	URTSDKMapModDefinition* mapmod = nullptr;
	if (MapModsByName.Contains(inMapMod))
	{
		mapmod = MapModsByName[inMapMod];
	}
	else
	{
		return TArray<FString>();
	}

	TArray<URTSDKMutatorDefinition*> mutators;
	mutators.Empty(inMutators.Num());
	for (int32 i = 0; i < inMutators.Num(); i++)
	{
		if (MutatorsByName.Contains(inMutators[i]))
		{
			mutators.Add(MutatorsByName[inMutators[i]]);
		}
		else
		{
			return TArray<FString>();
		}
	}
	return GetAllGameFeaturesForMods(gamemod, factionmods, mapmod, mutators);
}
