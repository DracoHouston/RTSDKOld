// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKSimState.h"
#include "RTSDKConstants.h"
#include "FixedPointTypes.h"
#include "RTSDKCommanderState.h"
#include "RTSDKTeamState.h"
#include "RTSDKForceState.h"
#include "RTSDKGameSimSubsystem.h"
#include "RTSDKPlayerControllerInterface.h"
#include "GameFramework/PlayerState.h"
#include "RTSDKWorldSettings.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameSession.h"
#include "RTSDKDeveloperSettings.h"

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
            OptionsString = FRTSDKLaunchOptionsHelpers::GetLaunchOptionsFromPIEMatchSetup(ws->PIEMatchSetup);
        }
    }
#endif
    //pull each team, force and commander declaration out of options

    TMap<int32, FRTSDKStateSetupInfo> TeamOptionsMap;
    TMap<int32, FRTSDKStateSetupInfo> ForceOptionsMap;
    TMap<int32, FRTSDKStateSetupInfo> CommanderOptionsMap;
    TMap<FString, FString> OtherOptionsMap;
    if (FRTSDKLaunchOptionsHelpers::ExtractOptions(OptionsString, OtherOptionsMap, TeamOptionsMap, ForceOptionsMap, CommanderOptionsMap))
    {
        OptionsMap = OtherOptionsMap;
        SetupTeams(TeamOptionsMap);
        SetupForces(ForceOptionsMap);
        SetupCommanders(CommanderOptionsMap);
    }
}

ERTSDKPreMatchTickResult ARTSDKSimStateBase::PreMatchTick()
{
    if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer)
    {
        return ERTSDKPreMatchTickResult::Ready;
    }
    //acquire local player controller
    if (LocalPlayerController == nullptr)
    {
        APlayerController* epicpc = GetWorld()->GetFirstPlayerController();
       
        if (epicpc == nullptr)
        {
            return ERTSDKPreMatchTickResult::Waiting;
        }
        if (epicpc->Implements<URTSDKPlayerControllerInterface>())
        {
            LocalPlayerController = epicpc;
        }
        else
        {
            return ERTSDKPreMatchTickResult::Waiting;
        }
    }
    return ERTSDKPreMatchTickResult::Ready;
}

void ARTSDKSimStateBase::BeginPlay()
{
    Super::BeginPlay();
    //set sim state in the subsystem locally for clients and begin running
    if (GetLocalRole() != ENetRole::ROLE_Authority)
    {
        URTSDKGameSimSubsystem* sim = GetWorld()->GetSubsystem<URTSDKGameSimSubsystem>();
        sim->SetSimState(this);
        sim->SetSimIsRunning(true);
        SimSubsystem = sim;
    }
}

ARTSDKSimStateSPOnly::ARTSDKSimStateSPOnly(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bReplicates = false;
}

void ARTSDKSimStateSPOnly::SetupTeams(const TMap<int32, FRTSDKStateSetupInfo>& inOptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKTeamStateBase*> teams;

    const URTSDKDeveloperSettings* RTSDKSettings = GetDefault<URTSDKDeveloperSettings>();
    for (auto It = inOptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKTeamStateBase* team = world->SpawnActor<ARTSDKTeamStateBase>(RTSDKSettings->TeamStateClass);
        team->Setup(It->Value, this, SimSubsystem);
        teams.Add(team);
    }
    SetTeams(teams);
}

void ARTSDKSimStateSPOnly::SetupForces(const TMap<int32, FRTSDKStateSetupInfo>& inOptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKForceStateBase*> forces;

    const URTSDKDeveloperSettings* RTSDKSettings = GetDefault<URTSDKDeveloperSettings>();
    for (auto It = inOptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKForceStateBase* force = world->SpawnActor<ARTSDKForceStateBase>(RTSDKSettings->ForceStateClass);
        force->Setup(It->Value, this, SimSubsystem);
        forces.Add(force);
    }
    SetForces(forces);
}

void ARTSDKSimStateSPOnly::SetupCommanders(const TMap<int32, FRTSDKStateSetupInfo>& inOptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKCommanderStateBase*> commanders;

    const URTSDKDeveloperSettings* RTSDKSettings = GetDefault<URTSDKDeveloperSettings>();
    for (auto It = inOptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKCommanderStateBase* commander = world->SpawnActor<ARTSDKCommanderStateBase>(RTSDKSettings->CommanderStateClass);
        commander->Setup(It->Value, this, SimSubsystem);
        commanders.Add(commander);
    }
    SetCommanders(commanders);
}

ERTSDKPreMatchTickResult ARTSDKSimStateSPOnly::PreMatchTick()
{
    //if there isn't a local player controller yet and it isn't a dedicated server (because this is singleplayer only (but also how do we not have a player controller?))
    //this call does set the local player controller, though, if not already set.
    if (Super::PreMatchTick() == ERTSDKPreMatchTickResult::Waiting)
    {
        return ERTSDKPreMatchTickResult::Waiting;
    }
    IRTSDKPlayerControllerInterface* localpc = Cast<IRTSDKPlayerControllerInterface>(LocalPlayerController);
    //acquire the local player's commander, the first non-bot commander found.
    if (LocalCommander == nullptr)
    {
        bool foundplayercommander = false;
        for (int32 i = 0; i < Commanders.Num(); i++)
        {
            if (Commanders[i]->GetIsPlayer())
            {
                LocalCommander = Commanders[i];
                localpc->SetCommanderState(LocalCommander);
                foundplayercommander = true;
                break;
            }
        }
        if (!foundplayercommander)
        {
            //observing a bot on bot skirmish or something weird like that, start if local pc WantsToBeReady.
            return localpc->GetWantsToBeReady() ? ERTSDKPreMatchTickResult::Ready : ERTSDKPreMatchTickResult::Waiting;
        }
    }
    //ready check
    return LocalCommander->GetIsReady() ? ERTSDKPreMatchTickResult::Ready : ERTSDKPreMatchTickResult::Waiting;
}

void ARTSDKSimStateSPOnly::SetMatchHasStarted(bool inMatchHasStarted)
{
    bMatchHasStarted = inMatchHasStarted;
}

bool ARTSDKSimStateSPOnly::GetMatchHasStarted()
{
    return bMatchHasStarted;
}

void ARTSDKSimStateSPOnly::SetMatchIsPaused(bool inMatchIsPaused)
{
    bMatchIsPaused = inMatchIsPaused;
}

bool ARTSDKSimStateSPOnly::GetMatchIsPaused()
{
    return bMatchIsPaused;
}

void ARTSDKSimStateSPOnly::RequestPause(AController* inController)
{
    //just pause and unpause when the player asks for it
    SetMatchIsPaused(true);
}

void ARTSDKSimStateSPOnly::RequestUnpause(AController* inController)
{
    //just pause and unpause when the player asks for it
    SetMatchIsPaused(false);
}

ERTSDKShouldAdvanceInputTurnResult ARTSDKSimStateSPOnly::ShouldAdvanceInputTurn()
{
    //turns happen every frame
    return ERTSDKShouldAdvanceInputTurnResult::Advance;
}

void ARTSDKSimStateSPOnly::OnPreAdvanceInputTurn()
{
    for (int32 i = 0; i < Commanders.Num(); i++)
    {
        Commanders[i]->FlushCommandBuffer();
    }
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

void ARTSDKSimStateServerClientLockstep::Setup(URTSDKGameSimSubsystem* inSimSubsystem, UWorld* inWorld)
{
    Super::Setup(inSimSubsystem, inWorld);
    const URTSDKDeveloperSettings* RTSDKSettings = GetDefault<URTSDKDeveloperSettings>();
    if (inWorld->GetNetMode() == ENetMode::NM_Standalone)
    {
        MinTurnDuration = 0.0;
    }
    else if (OptionsMap.Contains(TEXT("bIsLanMatch")))
    {
        MinTurnDuration = RTSDKSettings->MinimumNetTurnDuration;
    }
    else
    {
        MinTurnDuration = RTSDKSettings->MinimumNetTurnDuration;
    }
    LockstepTimeoutTurnCount = RTSDKSettings->LockstepTimeoutTurnCount;
    FramesPerTurn = CalculateFramesPerTurn();
}

void ARTSDKSimStateServerClientLockstep::SetupTeams(const TMap<int32, FRTSDKStateSetupInfo>& inOptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKTeamStateBase*> teams;

    const URTSDKDeveloperSettings* RTSDKSettings = GetDefault<URTSDKDeveloperSettings>();
    for (auto It = inOptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKTeamStateBase* team = world->SpawnActor<ARTSDKTeamStateBase>(RTSDKSettings->TeamStateClass);
        team->Setup(It->Value, this, SimSubsystem);
        teams.Add(team);
    }
    SetTeams(teams);
}

void ARTSDKSimStateServerClientLockstep::SetupForces(const TMap<int32, FRTSDKStateSetupInfo>& inOptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKForceStateBase*> forces;

    const URTSDKDeveloperSettings* RTSDKSettings = GetDefault<URTSDKDeveloperSettings>();
    for (auto It = inOptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKForceStateBase* force = world->SpawnActor<ARTSDKForceStateBase>(RTSDKSettings->ForceStateClass);
        force->Setup(It->Value, this, SimSubsystem);
        forces.Add(force);
    }
    SetForces(forces);
}

void ARTSDKSimStateServerClientLockstep::SetupCommanders(const TMap<int32, FRTSDKStateSetupInfo>& inOptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKCommanderStateBase*> commanders;

    const URTSDKDeveloperSettings* RTSDKSettings = GetDefault<URTSDKDeveloperSettings>();
    for (auto It = inOptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKCommanderStateBase* commander = world->SpawnActor<ARTSDKCommanderStateBase>(RTSDKSettings->CommanderStateClass);
        commander->Setup(It->Value, this, SimSubsystem);
        commanders.Add(commander);
    }
    SetCommanders(commanders);
}

ERTSDKPreMatchTickResult ARTSDKSimStateServerClientLockstep::PreMatchTick()
{
    //if there isn't a local player controller yet and it isn't a dedicated server. both unlikely,
    //but doing this ensures it is set, on anything but dedicated servers, who have no local players
    if (Super::PreMatchTick() == ERTSDKPreMatchTickResult::Waiting)
    {
        return ERTSDKPreMatchTickResult::Waiting;
    }

    IRTSDKPlayerControllerInterface* localpc = Cast<IRTSDKPlayerControllerInterface>(LocalPlayerController);
    UWorld* world = GetWorld();
    ENetMode netmode = world->GetNetMode();
#if WITH_EDITORONLY_DATA
    if ((world->WorldType == EWorldType::PIE) && (netmode != ENetMode::NM_Client))
    {
        int32 i = 0;
        for (FConstPlayerControllerIterator Iterator = world->GetPlayerControllerIterator(); Iterator; ++Iterator)
        {
            APlayerController* PlayerController = Iterator->Get();
            if (PlayerController->PlayerState != nullptr)
            {
                PlayerController->PlayerState->SetPlayerId(i);
                i++;
            }
            else
            {
                return ERTSDKPreMatchTickResult::Waiting;
            }
        }
        int32 playercommanders = 0;
        for (int32 c = 0; c < Commanders.Num(); c++)
        {
            playercommanders = Commanders[c]->GetIsPlayer() ? playercommanders + 1 : playercommanders;
        }
        if (i < playercommanders)
        {
            return ERTSDKPreMatchTickResult::Waiting;
        }
    }

#endif

    //acquire the local player's commander, one with the same PlayerID. but not on dedicated servers, who have no local player.
    if ((LocalCommander == nullptr) && (netmode != ENetMode::NM_DedicatedServer))
    {
        bool foundplayercommander = false;
        APlayerState* ps = LocalPlayerController->GetPlayerState<APlayerState>();
        if (ps == nullptr)//not yet replicated
        {
            return ERTSDKPreMatchTickResult::Waiting;
        }
        for (int32 i = 0; i < Commanders.Num(); i++)
        {
            if (Commanders[i] == nullptr)//not yet replicated 
            {
                return ERTSDKPreMatchTickResult::Waiting;
            }
            if (!Commanders[i]->GetIsPlayer())
            {
                continue;
            }
            foundplayercommander = true;
            if (Commanders[i]->GetPlayerID() == ps->GetPlayerId())
            {
                LocalCommander = Commanders[i];
                localpc->SetCommanderState(LocalCommander);
                break;
            }
        }
        if (!foundplayercommander)
        {
            //observing a bot on bot skirmish or something weird like that, start if local pc WantsToBeReady.
            return localpc->GetWantsToBeReady() ? ERTSDKPreMatchTickResult::Ready : ERTSDKPreMatchTickResult::Waiting;
        }
    }
    //here clients need to just wait for the flag to replicate over to exit pre match
    if (netmode == ENetMode::NM_Client)
    {
        return ERTSDKPreMatchTickResult::Waiting;
    }
    //if some sort of server, other than standalone, which is singleplayer, we try to set all commanders for players
    if ((netmode == ENetMode::NM_DedicatedServer) || (netmode == ENetMode::NM_ListenServer))
    {
        for (FConstPlayerControllerIterator Iterator = world->GetPlayerControllerIterator(); Iterator; ++Iterator)
        {
            APlayerController* PlayerController = Iterator->Get();
            IRTSDKPlayerControllerInterface* pc = Cast<IRTSDKPlayerControllerInterface>(PlayerController);
            if (pc == nullptr)
            {
                continue;
            }
            //ignore anyone that has theirs set already.
            if (pc->GetCommanderState() != nullptr)
            {
                continue;
            }

            for (int32 i = 0; i < Commanders.Num(); i++)
            {
                if (!Commanders[i]->GetIsPlayer())
                {
                    continue;
                }
                if (Commanders[i]->GetPlayerID() == PlayerController->GetPlayerState<APlayerState>()->GetPlayerId())
                {
                    pc->SetCommanderState(Commanders[i]);
                }
            }
        }
    }

    //ready check
    bool foundunready = false;
    for (int32 i = 0; i < Commanders.Num(); i++)
    {
        if (!Commanders[i]->GetIsReady())
        {
            foundunready = true;
            break;
        }
    }

    return foundunready ? ERTSDKPreMatchTickResult::Waiting : ERTSDKPreMatchTickResult::Ready;
}

void ARTSDKSimStateServerClientLockstep::SetMatchHasStarted(bool inMatchHasStarted)
{
    if (GetLocalRole() == ENetRole::ROLE_Authority)
    {
        bMatchHasStarted = inMatchHasStarted;
    }
}

bool ARTSDKSimStateServerClientLockstep::GetMatchHasStarted()
{
    //clients need to wait for things to replicate over, this is particularly true of PIE matches, which auto ready-up players
    if (GetLocalRole() != ENetRole::ROLE_Authority)
    {
        for (int32 i = 0; i < Teams.Num(); i++)
        {
            if (Teams[i] == nullptr)
            {
                return false;
            }
        }
        for (int32 i = 0; i < Forces.Num(); i++)
        {
            if (Forces[i] == nullptr)
            {
                return false;
            }
        }
        for (int32 i = 0; i < Forces.Num(); i++)
        {
            if (Commanders[i] == nullptr)
            {
                return false;
            }
        }
        if (LocalPlayerController == nullptr)
        {
            return false;
        }
    }
    return bMatchHasStarted;
}

void ARTSDKSimStateServerClientLockstep::SetMatchIsPaused(bool inMatchIsPaused)
{
    bMatchIsPaused = inMatchIsPaused;
}

bool ARTSDKSimStateServerClientLockstep::GetMatchIsPaused()
{
    return bMatchIsPaused;
}

void ARTSDKSimStateServerClientLockstep::RequestPause(AController* inController)
{
    ARTSDKCommanderStateBase* cmdr = nullptr;
    if (GetLocalRole() == ENetRole::ROLE_Authority)
    {
        if (PlayerMayRequestPause(inController, cmdr))
        {
            if (GetMatchIsPaused())
            {
                if (!Pausers.Contains(cmdr))
                {
                    Pausers.Add(cmdr);
                }
            }
            else
            {
                PauseCommands.AddPauseCommand(SimSubsystem->GetCurrentInputTurn() + 2);
                if (!Pausers.Contains(cmdr))
                {
                    Pausers.Add(cmdr);
                }
            }
        }
    }
}

void ARTSDKSimStateServerClientLockstep::RequestUnpause(AController* inController)
{
    ARTSDKCommanderStateBase* cmdr = nullptr;
    if (GetLocalRole() == ENetRole::ROLE_Authority)
    {
        if (PlayerMayRequestUnpause(inController, cmdr))
        {
            Pausers.Remove(cmdr);
            if (Pausers.Num() == 0)
            {
                //unpause everyone
                Multicast_OnUnpauseMatch();
            }
        }
    }
}

ERTSDKShouldAdvanceInputTurnResult ARTSDKSimStateServerClientLockstep::ShouldAdvanceInputTurn()
{
    if (GetMatchIsPaused())
    {
        return ERTSDKShouldAdvanceInputTurnResult::Wait;
    }
    int32 framecount = SimSubsystem->GetFrameCount();
    UWorld* world = GetWorld();
    //if we are in singleplayer advance every frame.
    //todo: replay playback check here for not advancing because of standalone
    if (world->GetNetMode() == ENetMode::NM_Standalone)
    {
        return ERTSDKShouldAdvanceInputTurnResult::Advance;
    }
    if ((LastTurnFrame + FramesPerTurn) <= framecount)
    {
        if (world->GetNetMode() == ENetMode::NM_Client)
        {
            for (int32 i = 0; i < Commanders.Num(); i++)
            {
                if (!Commanders[i]->HasTurn(SimSubsystem->GetCurrentInputTurn()))
                {
                    return ERTSDKShouldAdvanceInputTurnResult::Wait;
                }
            }
        }
        else
        {
            //check if any participating players have fallen behind. 
            for (int32 i = 0; i < Commanders.Num(); i++)
            {
                if (!Commanders[i]->GetIsPlayer())
                {
                    continue;
                }
                if (SimSubsystem->GetCurrentInputTurn() - Commanders[i]->GetLastCompletedTurn() >= LockstepTimeoutTurnCount)
                {
                    return ERTSDKShouldAdvanceInputTurnResult::Wait;
                }
            }
        }
        LastTurnFrame = framecount;
        return ERTSDKShouldAdvanceInputTurnResult::Advance;
    }
    //we are within a turn and unpaused, advance frame, we aren't ready to advance turn.
    return ERTSDKShouldAdvanceInputTurnResult::Skip;
}

void ARTSDKSimStateServerClientLockstep::OnPreAdvanceInputTurn()
{
    ENetMode netmode = GetWorld()->GetNetMode();
    if (netmode != ENetMode::NM_Client)
    {
        for (int32 i = 0; i < Commanders.Num(); i++)
        {
            Commanders[i]->FlushCommandBuffer();
        }
    }
    int32 currentturn = SimSubsystem->GetCurrentInputTurn();
    //if we have a pause command we execute that now.
    //Turn advancement will return wait until unpaused.
    if (PauseCommands.HasPauseCommandOnTurn(currentturn))
    {
        SetMatchIsPaused(true);
    }

    if (netmode == ENetMode::NM_DedicatedServer)
    {
        return;//no local player controller.
    }
    //todo, generate checksum for desync checks
    Cast<IRTSDKPlayerControllerInterface>(LocalPlayerController)->FinishInputTurn(currentturn, 0);
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

bool ARTSDKSimStateServerClientLockstep::PlayerMayRequestPause(AController* inController, ARTSDKCommanderStateBase*& outCommander)
{
    //default implementation allows free pausing and unpausing for participants, not spectators.
    IRTSDKPlayerControllerInterface* pc = Cast<IRTSDKPlayerControllerInterface>(inController);
    if (pc != nullptr)
    {
        ARTSDKCommanderStateBase* cmdr = pc->GetCommanderState();
        if (cmdr != nullptr)
        {
            outCommander = cmdr;
            return true;
        }
    }
    return false;
}

bool ARTSDKSimStateServerClientLockstep::PlayerMayRequestUnpause(AController* inController, ARTSDKCommanderStateBase*& outCommander)
{
    //we can't unpause a match that isn't paused.
    if (!GetMatchIsPaused())
    {
        return false;
    }
    //default implementation allows free pausing and unpausing for participants, not spectators.
    IRTSDKPlayerControllerInterface* pc = Cast<IRTSDKPlayerControllerInterface>(inController);
    if (pc != nullptr)
    {
        ARTSDKCommanderStateBase* cmdr = pc->GetCommanderState();
        if (cmdr != nullptr)
        {
            //only if they are an existing pauser
            if (Pausers.Contains(cmdr))
            {
                outCommander = cmdr;
                return true;
            }
        }
    }
    return false;
}

void ARTSDKSimStateServerClientLockstep::Multicast_OnUnpauseMatch_Implementation()
{
    SetMatchIsPaused(false);
}

int32 ARTSDKSimStateServerClientLockstep::CalculateFramesPerTurn()
{
    //todo: interop module for deterministic maths, this is starting to get annoying!
    FFixed64 durationoverscaledtimestep = MinTurnDuration / FFixed64((double)SimSubsystem->GetTimestep() * (double)SimSubsystem->GetTimeScale());
    return FFixedPointMath::CeilToInt(durationoverscaledtimestep);
}

void ARTSDKSimStateServerClientLockstep::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARTSDKSimStateServerClientLockstep, FramesPerTurn);
    DOREPLIFETIME(ARTSDKSimStateServerClientLockstep, MinTurnDuration);
    DOREPLIFETIME(ARTSDKSimStateServerClientLockstep, Commanders);
    DOREPLIFETIME(ARTSDKSimStateServerClientLockstep, Teams);
    DOREPLIFETIME(ARTSDKSimStateServerClientLockstep, Forces);
    DOREPLIFETIME(ARTSDKSimStateServerClientLockstep, bMatchHasStarted);
    DOREPLIFETIME(ARTSDKSimStateServerClientLockstep, PauseCommands);
    DOREPLIFETIME(ARTSDKSimStateServerClientLockstep, Pausers);
}

ARTSDKSimStateServerClientCurves::ARTSDKSimStateServerClientCurves(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bReplicates = true;
    bAlwaysRelevant = true;
}

void ARTSDKSimStateServerClientCurves::SetupTeams(const TMap<int32, FRTSDKStateSetupInfo>& inOptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKTeamStateBase*> teams;

    const URTSDKDeveloperSettings* RTSDKSettings = GetDefault<URTSDKDeveloperSettings>();
    for (auto It = inOptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKTeamStateBase* team = world->SpawnActor<ARTSDKTeamStateBase>(RTSDKSettings->TeamStateClass);
        team->Setup(It->Value, this, SimSubsystem);
        teams.Add(team);
    }
    SetTeams(teams);
}

void ARTSDKSimStateServerClientCurves::SetupForces(const TMap<int32, FRTSDKStateSetupInfo>& inOptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKForceStateBase*> forces;

    const URTSDKDeveloperSettings* RTSDKSettings = GetDefault<URTSDKDeveloperSettings>();
    for (auto It = inOptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKForceStateBase* force = world->SpawnActor<ARTSDKForceStateBase>(RTSDKSettings->ForceStateClass);
        force->Setup(It->Value, this, SimSubsystem);
        forces.Add(force);
    }
    SetForces(forces);
}

void ARTSDKSimStateServerClientCurves::SetupCommanders(const TMap<int32, FRTSDKStateSetupInfo>& inOptionsMap)
{
    UWorld* world = GetWorld();
    TArray<ARTSDKCommanderStateBase*> commanders;

    const URTSDKDeveloperSettings* RTSDKSettings = GetDefault<URTSDKDeveloperSettings>();
    for (auto It = inOptionsMap.CreateConstIterator(); It; ++It)
    {
        ARTSDKCommanderStateBase* commander = world->SpawnActor<ARTSDKCommanderStateBase>(RTSDKSettings->CommanderStateClass);
        commander->Setup(It->Value, this, SimSubsystem);
        commanders.Add(commander);
    }
    SetCommanders(commanders);
}

void ARTSDKSimStateServerClientCurves::SetMatchHasStarted(bool inMatchHasStarted)
{
    if (GetLocalRole() == ENetRole::ROLE_Authority)
    {
        bMatchHasStarted = inMatchHasStarted;
    }
}

bool ARTSDKSimStateServerClientCurves::GetMatchHasStarted()
{
    return bMatchHasStarted;
}

void ARTSDKSimStateServerClientCurves::SetMatchIsPaused(bool inMatchIsPaused)
{
    if (GetLocalRole() == ENetRole::ROLE_Authority)
    {
        bMatchIsPaused = inMatchIsPaused;
    }
}

bool ARTSDKSimStateServerClientCurves::GetMatchIsPaused()
{
    return bMatchIsPaused;
}

void ARTSDKSimStateServerClientCurves::RequestPause(AController* inController)
{
}

void ARTSDKSimStateServerClientCurves::RequestUnpause(AController* inController)
{
}

ERTSDKShouldAdvanceInputTurnResult ARTSDKSimStateServerClientCurves::ShouldAdvanceInputTurn()
{
    //we just advance turn every frame because only server runs commands.
    return ERTSDKShouldAdvanceInputTurnResult::Advance;
}

void ARTSDKSimStateServerClientCurves::OnPreAdvanceInputTurn()
{
    if (GetWorld()->GetNetMode() != NM_Client)
    {
        for (int32 i = 0; i < Commanders.Num(); i++)
        {
            Commanders[i]->FlushCommandBuffer();
        }
    }
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
    DOREPLIFETIME(ARTSDKSimStateServerClientCurves, bMatchHasStarted);
    DOREPLIFETIME(ARTSDKSimStateServerClientCurves, bMatchIsPaused);
    DOREPLIFETIME(ARTSDKSimStateServerClientCurves, Pausers);
}

void FRTSDKLockstepPauseCommand::PreReplicatedRemove(const FRTSDKLockstepPauseCommands& InArraySerializer)
{
}

void FRTSDKLockstepPauseCommand::PostReplicatedAdd(const FRTSDKLockstepPauseCommands& InArraySerializer)
{
}

void FRTSDKLockstepPauseCommand::PostReplicatedChange(const FRTSDKLockstepPauseCommands& InArraySerializer)
{
}

void FRTSDKLockstepPauseCommands::AddPauseCommand(int32 inTurn)
{
    FRTSDKLockstepPauseCommand cmd;
    cmd.Turn = inTurn;
    int32 Idx = PauseCommands.Add(cmd);
    MarkItemDirty(PauseCommands[Idx]);

    // server calls "on rep" also
    PauseCommands[Idx].PostReplicatedAdd(*this);
}

bool FRTSDKLockstepPauseCommands::HasPauseCommandOnTurn(int32 inTurn)
{
    for (int32 i = 0; i < PauseCommands.Num(); i++)
    {
        if (PauseCommands[i].Turn == inTurn)
        {
            return true;
        }
    }
    return false;
}
