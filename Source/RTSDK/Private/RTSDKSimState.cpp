// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKSimState.h"
#include "RTSDKCommanderState.h"
#include "RTSDKTeamState.h"
#include "RTSDKForceState.h"
#include "RTSDKGameSimSubsystem.h"
#include "RTSDKWorldSettings.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ARTSDKSimStateBase::ARTSDKSimStateBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = false;
}

void ARTSDKSimStateBase::Setup(URTSDKGameSimSubsystem* inSimSubsystem, UWorld* inWorld)
{
    SimSubsystem = inSimSubsystem;
    OptionsString = inWorld->GetAuthGameMode()->OptionsString;
#if WITH_EDITORONLY_DATA
    //PIE world settings based match setup
    //options string from url is ignored, rtsdk world settings 'PIEMatchSetup' property used instead
    if (inWorld->WorldType == EWorldType::PIE)
    {
        ARTSDKWorldSettings* ws = Cast<ARTSDKWorldSettings>(inWorld->GetWorldSettings());
        if (ws != nullptr)
        {
            FString PIELaunchOptions;
            int32 commanderidx = 0;
            int32 forceidx = 0;
            int32 humanidx = 0;
            //construct the new url options to pass to next part
            for (int32 t = 0; t < ws->PIEMatchSetup.Num(); t++)
            {
                PIELaunchOptions += FString::Printf(TEXT("?Team%i=|Name=%s"), t, *ws->PIEMatchSetup[t].DisplayName.ToString());
                PIELaunchOptions = ws->PIEMatchSetup[t].AdditionalOptions.IsEmpty() ? 
                    PIELaunchOptions : 
                    PIELaunchOptions + FString::Printf(TEXT("|%s)"), t, *ws->PIEMatchSetup[t].AdditionalOptions);
                for (int32 f = 0; f < ws->PIEMatchSetup[t].PIEForces.Num(); f++)
                {
                    PIELaunchOptions += FString::Printf(TEXT("?Force%i=|Name=%s|Team=%i"), forceidx, *ws->PIEMatchSetup[t].PIEForces[f].DisplayName.ToString(), t);
                    PIELaunchOptions = ws->PIEMatchSetup[t].PIEForces[f].AdditionalOptions.IsEmpty() ?
                        PIELaunchOptions :
                        PIELaunchOptions + FString::Printf(TEXT("|%s"), t, *ws->PIEMatchSetup[t].PIEForces[f].AdditionalOptions);
                    for (int32 c = 0; c < ws->PIEMatchSetup[t].PIEForces[f].PIECommanders.Num(); c++)
                    {
                        PIELaunchOptions = ws->PIEMatchSetup[t].PIEForces[f].PIECommanders[c].bIsBot ?
                            PIELaunchOptions + FString::Printf(TEXT("?Commander%i=|Name=%s|Force=%i|IsBot"), commanderidx, *ws->PIEMatchSetup[t].PIEForces[f].PIECommanders[c].DisplayName.ToString(), t, f) :
                            PIELaunchOptions + FString::Printf(TEXT("?Commander%i=|Name=%s|Force=%i|Player=%i"), commanderidx, *ws->PIEMatchSetup[t].PIEForces[f].PIECommanders[c].DisplayName.ToString(), t, f, humanidx);
                        PIELaunchOptions = ws->PIEMatchSetup[t].PIEForces[f].PIECommanders[c].AdditionalOptions.IsEmpty() ?
                            PIELaunchOptions :
                            PIELaunchOptions + FString::Printf(TEXT("|%s"), t, *ws->PIEMatchSetup[t].PIEForces[f].PIECommanders[c].AdditionalOptions);
                        commanderidx++;
                        humanidx = ws->PIEMatchSetup[t].PIEForces[f].PIECommanders[c].bIsBot ? humanidx : humanidx + 1;
                    }
                    forceidx++;
                }
            }
            OptionsString = PIELaunchOptions;
        }
    }
#endif
    //pull each team, force and commander declaration out of options
    TMap<int32, FRTSDKStateSetupInfo> TeamOptionsMap;
    TMap<int32, FRTSDKStateSetupInfo> ForceOptionsMap;
    TMap<int32, FRTSDKStateSetupInfo> CommanderOptionsMap;
    FString LaunchOptions = OptionsString;
    FString Pair, PairKey, PairValue;
    while (UGameplayStatics::GrabOption(LaunchOptions, Pair))
    {
        UGameplayStatics::GetKeyValue(Pair, PairKey, PairValue);
        if (PairKey.StartsWith(TEXT("Team")))
        {
            FString idxstring = PairKey.RightChop(4);
            if (idxstring.IsNumeric())
            {
                int32 idx = FCString::Atoi(*idxstring);
                FString SubOptions = PairValue.Replace(TEXT("|"), TEXT("?"));
                FString SubPair, SubPairKey, SubPairValue;
                FRTSDKStateSetupInfo SubParams;
                while (UGameplayStatics::GrabOption(SubOptions, SubPair))
                {
                    UGameplayStatics::GetKeyValue(SubPair, SubPairKey, SubPairValue);
                    SubParams.OptionsMap.Add(SubPairKey, SubPairValue);
                }
                TeamOptionsMap.Add(idx, SubParams);
            }
        }
        else if (PairKey.StartsWith(TEXT("Force")))
        {
            FString idxstring = PairKey.RightChop(5);
            if (idxstring.IsNumeric())
            {
                int32 idx = FCString::Atoi(*idxstring);
                FString SubOptions = PairValue.Replace(TEXT("|"), TEXT("?"));
                FString SubPair, SubPairKey, SubPairValue;
                FRTSDKStateSetupInfo SubParams;
                while (UGameplayStatics::GrabOption(SubOptions, SubPair))
                {
                    UGameplayStatics::GetKeyValue(SubPair, SubPairKey, SubPairValue);
                    SubParams.OptionsMap.Add(SubPairKey, SubPairValue);
                }
                ForceOptionsMap.Add(idx, SubParams);
            }
        }
        else if (PairKey.StartsWith(TEXT("Commander")))
        {
            FString idxstring = PairKey.RightChop(9);
            if (idxstring.IsNumeric())
            {
                int32 idx = FCString::Atoi(*idxstring);
                FString SubOptions = PairValue.Replace(TEXT("|"), TEXT("?"));
                FString SubPair, SubPairKey, SubPairValue;
                FRTSDKStateSetupInfo SubParams;
                while (UGameplayStatics::GrabOption(SubOptions, SubPair))
                {
                    UGameplayStatics::GetKeyValue(SubPair, SubPairKey, SubPairValue);
                    SubParams.OptionsMap.Add(SubPairKey, SubPairValue);
                }
                CommanderOptionsMap.Add(idx, SubParams);
            }
        }
    }
    //now we can go through each declaration, they are sorted by index number first
    
    TeamOptionsMap.KeySort(TLess<int32>());
    ForceOptionsMap.KeySort(TLess<int32>());
    CommanderOptionsMap.KeySort(TLess<int32>());
    //continuous index check, this map is sorted by key, which is index, and should be 0-n with no gaps, or the url is malformed, regardless of order they were declared in
    //todo: log failures to let whoever malformed their open/servertravel url know about it.
    int32 idx = 0;
    for (auto It = TeamOptionsMap.CreateConstIterator(); It; ++It)
    {
        if (It->Key == idx)
        {
            idx++;
        }
        else
        {
            return;
        }
    }
    idx = 0;
    for (auto It = ForceOptionsMap.CreateConstIterator(); It; ++It)
    {
        if (It->Key == idx)
        {
            idx++;
        }
        else
        {
            return;
        }
    }
    idx = 0;
    for (auto It = CommanderOptionsMap.CreateConstIterator(); It; ++It)
    {
        if (It->Key == idx)
        {
            idx++;
        }
        else
        {
            return;
        }
    }
    SetupTeams(TeamOptionsMap);
    SetupForces(ForceOptionsMap);
    SetupCommanders(CommanderOptionsMap);
}

ARTSDKSimStateSPOnly::ARTSDKSimStateSPOnly(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bReplicates = false;
}

void ARTSDKSimStateSPOnly::SetupTeams(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKTeamStateBase*> teams;

    for (auto It = OptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKTeamStateBase* team = world->SpawnActor<ARTSDKTeamStateBase>(ARTSDKTeamStateSPOnly::StaticClass());
        team->Setup(It->Value, this, SimSubsystem);
        teams.Add(team);
    }
    SetTeams(teams);
}

void ARTSDKSimStateSPOnly::SetupForces(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKForceStateBase*> forces;

    for (auto It = OptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKForceStateBase* force = world->SpawnActor<ARTSDKForceStateBase>(ARTSDKForceStateSPOnly::StaticClass());
        force->Setup(It->Value, this, SimSubsystem);
        forces.Add(force);
    }
    SetForces(forces);
}

void ARTSDKSimStateSPOnly::SetupCommanders(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKCommanderStateBase*> commanders;

    for (auto It = OptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKCommanderStateBase* commander = world->SpawnActor<ARTSDKCommanderStateBase>(ARTSDKCommanderStateSPOnly::StaticClass());
        commander->Setup(It->Value, this, SimSubsystem);
        commanders.Add(commander);
    }
    SetCommanders(commanders);
}

ARTSDKCommanderStateBase* ARTSDKSimStateSPOnly::GetCommander(const int32& inCommanderID)
{
    if (inCommanderID < Commanders.Num())
    {
        return Commanders[inCommanderID];
    }
    return nullptr;
}

TArray<ARTSDKCommanderStateBase*> ARTSDKSimStateSPOnly::GetCommanders()
{
    return Commanders;
}

int32 ARTSDKSimStateSPOnly::AddCommander(ARTSDKCommanderStateBase* inCommanderState)
{
    return Commanders.Add(inCommanderState);
}

void ARTSDKSimStateSPOnly::SetCommanders(TArray<ARTSDKCommanderStateBase*> inCommanderStates)
{
    Commanders = inCommanderStates;
}

int32 ARTSDKSimStateSPOnly::GetCommanderCount()
{
    return Commanders.Num();
}

ARTSDKForceStateBase* ARTSDKSimStateSPOnly::GetForce(const int32& inForceID)
{
    if (inForceID < Forces.Num())
    {
        return Forces[inForceID];
    }
    return nullptr;
}

TArray<ARTSDKForceStateBase*> ARTSDKSimStateSPOnly::GetForces()
{
    return Forces;
}

int32 ARTSDKSimStateSPOnly::AddForce(ARTSDKForceStateBase* inForceState)
{
    return Forces.Add(inForceState);
}

void ARTSDKSimStateSPOnly::SetForces(TArray<ARTSDKForceStateBase*> inForceStates)
{
    Forces = inForceStates;
}

int32 ARTSDKSimStateSPOnly::GetForceCount()
{
    return Forces.Num();
}

ARTSDKTeamStateBase* ARTSDKSimStateSPOnly::GetTeam(const int32& inTeamID)
{
    if (inTeamID < Teams.Num())
    {
        return Teams[inTeamID];
    }
    return nullptr;
}

TArray<ARTSDKTeamStateBase*> ARTSDKSimStateSPOnly::GetTeams()
{
    return Teams;
}

int32 ARTSDKSimStateSPOnly::AddTeam(ARTSDKTeamStateBase* inTeamState)
{
    return Teams.Add(inTeamState);
}

void ARTSDKSimStateSPOnly::SetTeams(TArray<ARTSDKTeamStateBase*> inTeamStates)
{
    Teams = inTeamStates;
}

int32 ARTSDKSimStateSPOnly::GetTeamCount()
{
    return Teams.Num();
}

TArray<FRTSDKPlayerCommandReplicationInfo> ARTSDKSimStateSPOnly::GetCommandsForCommanderByTurn(const int32& inCommanderID, const int32& inTurn)
{
    if ((inCommanderID < Commanders.Num()) && (Commanders[inCommanderID] != nullptr))
    {
        return Commanders[inCommanderID]->GetCommandsByTurn(inTurn);
    }
    return TArray<FRTSDKPlayerCommandReplicationInfo>();
}

ARTSDKSimStateServerClientLockstep::ARTSDKSimStateServerClientLockstep(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bReplicates = true;
    bAlwaysRelevant = true;
}

void ARTSDKSimStateServerClientLockstep::SetupTeams(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKTeamStateBase*> teams;

    for (auto It = OptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKTeamStateBase* team = world->SpawnActor<ARTSDKTeamStateBase>(ARTSDKTeamStateServerClientLockstep::StaticClass());
        team->Setup(It->Value, this, SimSubsystem);
        teams.Add(team);
    }
    SetTeams(teams);
}

void ARTSDKSimStateServerClientLockstep::SetupForces(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKForceStateBase*> forces;

    for (auto It = OptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKForceStateBase* force = world->SpawnActor<ARTSDKForceStateBase>(ARTSDKForceStateServerClientLockstep::StaticClass());
        force->Setup(It->Value, this, SimSubsystem);
        forces.Add(force);
    }
    SetForces(forces);
}

void ARTSDKSimStateServerClientLockstep::SetupCommanders(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKCommanderStateBase*> commanders;

    for (auto It = OptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKCommanderStateBase* commander = world->SpawnActor<ARTSDKCommanderStateBase>(ARTSDKCommanderStateServerClientLockstep::StaticClass());
        commander->Setup(It->Value, this, SimSubsystem);
        commanders.Add(commander);
    }
    SetCommanders(commanders);
}

ARTSDKCommanderStateBase* ARTSDKSimStateServerClientLockstep::GetCommander(const int32& inCommanderID)
{
    if (inCommanderID < Commanders.Num())
    {
        return Commanders[inCommanderID];
    }
    return nullptr;
}

TArray<ARTSDKCommanderStateBase*> ARTSDKSimStateServerClientLockstep::GetCommanders()
{
    return Commanders;
}

int32 ARTSDKSimStateServerClientLockstep::AddCommander(ARTSDKCommanderStateBase* inCommanderState)
{
    return Commanders.Add(inCommanderState);
}

void ARTSDKSimStateServerClientLockstep::SetCommanders(TArray<ARTSDKCommanderStateBase*> inCommanderStates)
{
    Commanders = inCommanderStates;
}

int32 ARTSDKSimStateServerClientLockstep::GetCommanderCount()
{
    return Commanders.Num();
}

ARTSDKForceStateBase* ARTSDKSimStateServerClientLockstep::GetForce(const int32& inForceID)
{
    if (inForceID < Forces.Num())
    {
        return Forces[inForceID];
    }
    return nullptr;
}

TArray<ARTSDKForceStateBase*> ARTSDKSimStateServerClientLockstep::GetForces()
{
    return Forces;
}

int32 ARTSDKSimStateServerClientLockstep::AddForce(ARTSDKForceStateBase* inForceState)
{
    return Forces.Add(inForceState);
}

void ARTSDKSimStateServerClientLockstep::SetForces(TArray<ARTSDKForceStateBase*> inForceStates)
{
    Forces = inForceStates;
}

int32 ARTSDKSimStateServerClientLockstep::GetForceCount()
{
    return Forces.Num();
}

ARTSDKTeamStateBase* ARTSDKSimStateServerClientLockstep::GetTeam(const int32& inTeamID)
{
    if (inTeamID < Teams.Num())
    {
        return Teams[inTeamID];
    }
    return nullptr;
}

TArray<ARTSDKTeamStateBase*> ARTSDKSimStateServerClientLockstep::GetTeams()
{
    return Teams;
}

int32 ARTSDKSimStateServerClientLockstep::AddTeam(ARTSDKTeamStateBase* inTeamState)
{
    return Teams.Add(inTeamState);
}

void ARTSDKSimStateServerClientLockstep::SetTeams(TArray<ARTSDKTeamStateBase*> inTeamStates)
{
    Teams = inTeamStates;
}

int32 ARTSDKSimStateServerClientLockstep::GetTeamCount()
{
    return Teams.Num();
}

TArray<FRTSDKPlayerCommandReplicationInfo> ARTSDKSimStateServerClientLockstep::GetCommandsForCommanderByTurn(const int32& inCommanderID, const int32& inTurn)
{
    if ((inCommanderID < Commanders.Num()) && (Commanders[inCommanderID] != nullptr))
    {
        return Commanders[inCommanderID]->GetCommandsByTurn(inTurn);
    }
    return TArray<FRTSDKPlayerCommandReplicationInfo>();
}

void ARTSDKSimStateServerClientLockstep::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARTSDKSimStateServerClientLockstep, Commanders);
    DOREPLIFETIME(ARTSDKSimStateServerClientLockstep, Teams);
    DOREPLIFETIME(ARTSDKSimStateServerClientLockstep, Forces);
}

ARTSDKSimStateServerClientCurves::ARTSDKSimStateServerClientCurves(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bReplicates = true;
    bAlwaysRelevant = true;
}

void ARTSDKSimStateServerClientCurves::SetupTeams(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKTeamStateBase*> teams;

    for (auto It = OptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKTeamStateBase* team = world->SpawnActor<ARTSDKTeamStateBase>(ARTSDKTeamStateServerClientCurves::StaticClass());
        team->Setup(It->Value, this, SimSubsystem);
        teams.Add(team);
    }
    SetTeams(teams);
}

void ARTSDKSimStateServerClientCurves::SetupForces(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKForceStateBase*> forces;

    for (auto It = OptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKForceStateBase* force = world->SpawnActor<ARTSDKForceStateBase>(ARTSDKForceStateServerClientCurves::StaticClass());
        force->Setup(It->Value, this, SimSubsystem);
        forces.Add(force);
    }
    SetForces(forces);
}

void ARTSDKSimStateServerClientCurves::SetupCommanders(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKCommanderStateBase*> commanders;

    for (auto It = OptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKCommanderStateBase* commander = world->SpawnActor<ARTSDKCommanderStateBase>(ARTSDKCommanderStateServerClientCurves::StaticClass());
        commander->Setup(It->Value, this, SimSubsystem);
        commanders.Add(commander);
    }
    SetCommanders(commanders);
}

ARTSDKCommanderStateBase* ARTSDKSimStateServerClientCurves::GetCommander(const int32& inCommanderID)
{
    if (inCommanderID < Commanders.Num())
    {
        return Commanders[inCommanderID];
    }
    return nullptr;
}

TArray<ARTSDKCommanderStateBase*> ARTSDKSimStateServerClientCurves::GetCommanders()
{
    return Commanders;
}

int32 ARTSDKSimStateServerClientCurves::AddCommander(ARTSDKCommanderStateBase* inCommanderState)
{
    return Commanders.Add(inCommanderState);
}

void ARTSDKSimStateServerClientCurves::SetCommanders(TArray<ARTSDKCommanderStateBase*> inCommanderStates)
{
    Commanders = inCommanderStates;
}

int32 ARTSDKSimStateServerClientCurves::GetCommanderCount()
{
    return Commanders.Num();
}

ARTSDKForceStateBase* ARTSDKSimStateServerClientCurves::GetForce(const int32& inForceID)
{
    if (inForceID < Forces.Num())
    {
        return Forces[inForceID];
    }
    return nullptr;
}

TArray<ARTSDKForceStateBase*> ARTSDKSimStateServerClientCurves::GetForces()
{
    return Forces;
}

int32 ARTSDKSimStateServerClientCurves::AddForce(ARTSDKForceStateBase* inForceState)
{
    return Forces.Add(inForceState);
}

void ARTSDKSimStateServerClientCurves::SetForces(TArray<ARTSDKForceStateBase*> inForceStates)
{
    Forces = inForceStates;
}

int32 ARTSDKSimStateServerClientCurves::GetForceCount()
{
    return Forces.Num();
}

ARTSDKTeamStateBase* ARTSDKSimStateServerClientCurves::GetTeam(const int32& inTeamID)
{
    if (inTeamID < Teams.Num())
    {
        return Teams[inTeamID];
    }
    return nullptr;
}

TArray<ARTSDKTeamStateBase*> ARTSDKSimStateServerClientCurves::GetTeams()
{
    return Teams;
}

int32 ARTSDKSimStateServerClientCurves::AddTeam(ARTSDKTeamStateBase* inTeamState)
{
    return Teams.Add(inTeamState);
}

void ARTSDKSimStateServerClientCurves::SetTeams(TArray<ARTSDKTeamStateBase*> inTeamStates)
{
    Teams = inTeamStates;
}

int32 ARTSDKSimStateServerClientCurves::GetTeamCount()
{
    return Teams.Num();
}

TArray<FRTSDKPlayerCommandReplicationInfo> ARTSDKSimStateServerClientCurves::GetCommandsForCommanderByTurn(const int32& inCommanderID, const int32& inTurn)
{
    if ((inCommanderID < Commanders.Num()) && (Commanders[inCommanderID] != nullptr))
    {
        return Commanders[inCommanderID]->GetCommandsByTurn(inTurn);
    }
    return TArray<FRTSDKPlayerCommandReplicationInfo>();
}

void ARTSDKSimStateServerClientCurves::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARTSDKSimStateServerClientCurves, Commanders);
    DOREPLIFETIME(ARTSDKSimStateServerClientCurves, Teams);
    DOREPLIFETIME(ARTSDKSimStateServerClientCurves, Forces);
}