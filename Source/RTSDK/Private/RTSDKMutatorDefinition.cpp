// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKMutatorDefinition.h"
#include "RTSDKGameModDefinition.h"
#include "RTSDKModManager.h"
#include "GameFeaturesSubsystem.h"

void URTSDKMutatorDefinition::Init(FName inDevName, FText inDisplayName, FString inGameFeatureName,	TArray<FString> inAdditionalGameFeatureNames, FName inAssociatedGameModName)
{
	ModDevName = inDevName;
	ModDisplayName = inDisplayName;
	GameFeatureName = inGameFeatureName;
	AdditionalGameFeatureNames = inAdditionalGameFeatureNames;
	AssociatedGameModName = inAssociatedGameModName;
	bIsValid = false;
}

void URTSDKMutatorDefinition::BuildModDependencies(URTSDKModManager* inModManager)
{
	if (GameFeatureName.Len() <= 0)
	{
		bIsValid = false;
		return;
	}

	if (AssociatedGameModName.IsNone())
	{
		bIsValid = true;
		return;
	}
	else
	{
		URTSDKGameModDefinition* associatedgamemod = inModManager->GetGameModByName(AssociatedGameModName);
		if (associatedgamemod != nullptr)
		{
			AssociatedGameMod = associatedgamemod;
			bIsValid = true;
			return;
		}
		else
		{
			bIsValid = false;
			return;
		}
	}
}

void URTSDKMutatorDefinition::BuildMod(URTSDKModManager* inModManager)
{
	if (!bIsValid)
	{
		return;
	}
	
	TArray<FString> combinedfeaturenames;
	combinedfeaturenames.Empty(AdditionalGameFeatureNames.Num() + 1);
	combinedfeaturenames.Add(GameFeatureName);
	for (int32 i = 0; i < AdditionalGameFeatureNames.Num(); i++)
	{
		if ((AdditionalGameFeatureNames[i].Len() > 0) && (!combinedfeaturenames.Contains(AdditionalGameFeatureNames[i])))
		{
			combinedfeaturenames.Add(AdditionalGameFeatureNames[i]);
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
