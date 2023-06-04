// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKMapModDefinition.h"
#include "Engine/Level.h"
#include "RTSDKGameModDefinition.h"
#include "RTSDKModManager.h"
#include "GameFeaturesSubsystem.h"

void URTSDKMapModDefinition::Init(FName inDevName, FText inDisplayName, FString inGameFeatureName, FPrimaryAssetId inAssociatedLevel, FName inAssociatedGameModName, bool bInIsAbstractMod, FName inParentModName)
{
	ModDevName = inDevName;
	ModDisplayName = inDisplayName;
	GameFeatureName = inGameFeatureName;
	AssociatedLevel = inAssociatedLevel;
	AssociatedGameModName = inAssociatedGameModName;
	bIsAbstractMod = bInIsAbstractMod;
	ParentMapModName = inParentModName;
	bIsValid = false;
}

void URTSDKMapModDefinition::BuildModDependencies(URTSDKModManager* inModManager)
{
	if (GameFeatureName.Len() <= 0)
	{
		bIsValid = false;
		return;
	}

	if (AssociatedGameModName.IsNone())
	{
		bIsValid = false;
		return;
	}
	else
	{
		URTSDKGameModDefinition* associatedgamemod = inModManager->GetGameModByName(AssociatedGameModName);
		if (associatedgamemod != nullptr)
		{
			AssociatedGameMod = associatedgamemod;
		}
		else
		{
			bIsValid = false;
			return;
		}
	}

	if (!ParentMapModName.IsNone())
	{
		URTSDKMapModDefinition* parent = inModManager->GetMapModByName(ParentMapModName);
		if (parent != nullptr)
		{
			ParentMapMod = parent;
			bIsValid = true;
			return;
		}
	}
	else
	{
		bIsValid = true;
		return;
	}
}

void URTSDKMapModDefinition::BuildMod(URTSDKModManager* inModManager)
{
	if (!bIsValid)
	{
		return;
	}

	TArray<URTSDKMapModDefinition*> allparents;
	URTSDKMapModDefinition* currentouter = ParentMapMod;
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
		currentouter = currentouter->ParentMapMod;
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
}
