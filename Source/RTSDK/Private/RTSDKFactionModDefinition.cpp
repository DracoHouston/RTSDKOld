// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKFactionModDefinition.h"
#include "RTSDKGameModDefinition.h"
#include "RTSDKFactionDefinition.h"
#include "RTSDKModManager.h"
#include "GameFeaturesSubsystem.h"

void URTSDKFactionModDefinition::Init(FName inDevName, FText inDisplayName, FString inGameFeatureName, TSoftClassPtr<URTSDKFactionDefinition> inFactionClass, FName inAssociatedGameModName, bool bInIsAbstractMod, FName inParentFactionModName)
{
	ModDevName = inDevName;
	ModDisplayName = inDisplayName;
	GameFeatureName = inGameFeatureName;
	FactionClass = inFactionClass;
	AssociatedGameModName = inAssociatedGameModName;
	bIsAbstractMod = bInIsAbstractMod;
	ParentFactionModName = inParentFactionModName;
	bIsValid = false;
}

void URTSDKFactionModDefinition::BuildModDependencies(URTSDKModManager* inModManager)
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

	if (!ParentFactionModName.IsNone())
	{
		URTSDKFactionModDefinition* parent = inModManager->GetFactionModByName(ParentFactionModName);
		if (parent != nullptr)
		{
			ParentFactionMod = parent;
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

void URTSDKFactionModDefinition::BuildMod(URTSDKModManager* inModManager)
{
	if (!bIsValid)
	{
		return;
	}

	TArray<URTSDKFactionModDefinition*> allparents;
	URTSDKFactionModDefinition* currentouter = ParentFactionMod;
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
		currentouter = currentouter->ParentFactionMod;
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
