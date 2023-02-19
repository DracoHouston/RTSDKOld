// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

//#define RTSDK_USE_FIXED_POINT 0
//#define RTSDK_USE_SERVER_CLIENT 0

#if RTSDK_USE_FIXED_POINT == 0
#include "DeterministicFloatTypes.h"
using FRTSMath = FDeterministicFloatMath;
using FRTSNumber64 =	FDeterministicNumber64;
using FRTSNumber32 =	FDeterministicNumber32;
using FRTSVector64 =	FDeterministicVector64;
//using FRTSVector32 =	FVector3f;
using FRTSQuat64 =		FDeterministicQuat64;
//using FRTSQuat32 =		FQuat4f;
using FRTSTransform64 =	FDeterministicTransform64;
//using FRTSTransform32 = FTransform3f;
#else
#include "FixedPointTypes.h"
using FRTSMath =		FFixedPointMath;
using FRTSNumber64 =	FFixed64;
using FRTSNumber32 =	FFixed32;
using FRTSVector64 =	FFixedVector64;
//using FRTSVector32 =	FFixedVector32;
using FRTSQuat64 =		FFixedQuat64;
//using FRTSQuat32 =		FFixedQuat32;
using FRTSTransform64 = FFixedTransform64;
//using FRTSTransform32 = FFixedTransform32;
#endif

namespace RTSDK
{
	const FName SimProcessingPhaseName = FName(TEXT("RTSGameSim"));
}

//Constants for processor group names, for execution order
namespace UE::Mass::ProcessorGroupNames
{
	const FName RTSPreSim = FName(TEXT("RTSPreSim"));
	const FName RTSMovement = FName(TEXT("RTSMovement"));
	const FName RTSMovePreCommit = FName(TEXT("RTSMovePreCommit"));
	const FName RTSMoveCommit = FName(TEXT("RTSMoveCommit"));
	const FName RTSVisInterpolation = FName(TEXT("RTSVisInterpolation"));
}

namespace UE::Mass::Signals
{
	const FName RTSUnitMoved = FName(TEXT("RTSUnitMoved"));
}

template<typename T>
struct TAutoIncludeInRTSSimPipeline
{
	enum { Value = false };
};
