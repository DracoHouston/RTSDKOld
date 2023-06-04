// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKGameModDefinition.h"
#include "RTSDKFactionModDefinition.h"
#include "RTSDKMapModDefinition.h"
#include "RTSDKConfigurableHUDDefinition.h"
#include "RTSDKModManager.h"
#include "GameFeaturesSubsystem.h"

void URTSDKGameModDefinition::Init(FName inDevName, const FText& inDisplayName, const FString& inGameFeatureName, bool bInIsAbstract, FName inParentModName)
{
	ParentGameModName = inParentModName;
	ModDevName = inDevName;
	ModDisplayName = inDisplayName;
	GameFeatureName = inGameFeatureName;
	bIsAbstractMod = bInIsAbstract;
	bIsValid = false;
}

void URTSDKGameModDefinition::BuildModDependencies(URTSDKModManager* inModManager)
{
	if (GameFeatureName.Len() <= 0)
	{
		bIsValid = false;
		return;
	}
	if (!ParentGameModName.IsNone())
	{
		URTSDKGameModDefinition* parent = inModManager->GetGameModByName(ParentGameModName);
		if (parent != nullptr)
		{
			ParentGameMod = parent;
			bIsValid = true;
			return;
		}
		else
		{
			bIsValid = false;
			return;
		}
	}
	else
	{
		bIsValid = true;
		return;
	}
}

void URTSDKGameModDefinition::BuildMod(URTSDKModManager* inModManager)
{
	if (!bIsValid)
	{
		return;
	}
		
	TArray<URTSDKGameModDefinition*> allparents;
	URTSDKGameModDefinition* currentouter = ParentGameMod;
	while (currentouter != nullptr)
	{
		if (!currentouter->bIsValid)
		{
			bIsValid = false;//bad parent
			return;
		}
		if (allparents.Contains(currentouter))
		{
			bIsValid = false;//circular dependency
			return;
		}
		allparents.Add(currentouter);
		currentouter = currentouter->ParentGameMod;
	}

	TArray<FString> combinedfeaturenames;
	combinedfeaturenames.Empty(AdditionalGameFeatureNames.Num() + 1 + allparents.Num());
	combinedfeaturenames.Add(GameFeatureName);
	for (int32 i = 0; i < AdditionalGameFeatureNames.Num(); i++)
	{
		if ((AdditionalGameFeatureNames[i].Len() > 0) && (!combinedfeaturenames.Contains(AdditionalGameFeatureNames[i])))
		{
			combinedfeaturenames.Add(AdditionalGameFeatureNames[i]);
		}
	}

	for (int32 i = 0; i < allparents.Num(); i++)
	{
		if ((allparents[i]->GameFeatureName.Len() > 0) && (!combinedfeaturenames.Contains(allparents[i]->GameFeatureName)))
		{
			combinedfeaturenames.Add(allparents[i]->GameFeatureName);
		}
		for (int32 j = 0; j < allparents[i]->AdditionalGameFeatureNames.Num(); j++)
		{
			if ((allparents[i]->AdditionalGameFeatureNames[j].Len() > 0) && (!combinedfeaturenames.Contains(allparents[i]->AdditionalGameFeatureNames[j])))
			{
				combinedfeaturenames.Add(allparents[i]->AdditionalGameFeatureNames[j]);
			}
		}
	}
	UGameFeaturesSubsystem& gamefeatures = UGameFeaturesSubsystem::Get();
	CombinedGameFeatureURLs.Empty(combinedfeaturenames.Num());
	for (int32 i = 0; i < combinedfeaturenames.Num(); i++)
	{
		FString featureurl;
		if (gamefeatures.GetPluginURLByName(combinedfeaturenames[i], featureurl))
		{
			CombinedGameFeatureURLs.Add(featureurl);
		}
		else
		{
			//bad game feature name or missing required game feature
			bIsValid = false;
			return;
		}
	}

	if (bIsAbstractMod)
	{
		return;//abstract mods stop here
	}

	ValidMapMods.Empty();
	ValidFactionMods.Empty();
	ValidConfigurableHUDs.Empty();
	ValidMapMods += inModManager->GetMapModsByGameMod(this);
	ValidFactionMods += inModManager->GetFactionModsByGameMod(this);
	ValidConfigurableHUDs += inModManager->GetConfigurableHUDsByGameMod(this);
	for (int32 i = 0; i < allparents.Num(); i++)
	{
		ValidMapMods += inModManager->GetMapModsByGameMod(allparents[i]);
		ValidFactionMods += inModManager->GetFactionModsByGameMod(allparents[i]);
		ValidConfigurableHUDs += inModManager->GetConfigurableHUDsByGameMod(allparents[i]);
	}
}
