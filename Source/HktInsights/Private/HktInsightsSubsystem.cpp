// Copyright Hkt Studios, Inc. All Rights Reserved.

#include "HktInsightsSubsystem.h"
#include "HktCoreDataCollector.h"
#include "HktInsightsLog.h"
#include "Engine/World.h"
#include "TimerManager.h"

bool UHktInsightsWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if ENABLE_HKT_INSIGHTS
    return true;
#else
    return false;
#endif
}

void UHktInsightsWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    LastSeenVersion = FHktCoreDataCollector::Get().GetVersion();

    // 0.1초 간격으로 변경 감지 폴링
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            PollTimerHandle,
            FTimerDelegate::CreateUObject(this, &UHktInsightsWorldSubsystem::CheckForUpdates),
            0.1f,
            true);
    }

    UE_LOG(LogHktInsights, Log, TEXT("HktInsightsWorldSubsystem initialized"));
}

void UHktInsightsWorldSubsystem::Deinitialize()
{
    // 타이머 해제
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PollTimerHandle);
    }

    // 월드 종료 시 수집 데이터 청소
    FHktCoreDataCollector::Get().Clear();

    UE_LOG(LogHktInsights, Log, TEXT("HktInsightsWorldSubsystem deinitialized — data cleared"));

    Super::Deinitialize();
}

void UHktInsightsWorldSubsystem::CheckForUpdates()
{
    const uint32 CurrentVersion = FHktCoreDataCollector::Get().GetVersion();
    if (CurrentVersion != LastSeenVersion)
    {
        LastSeenVersion = CurrentVersion;
        OnDataChanged.Broadcast();
    }
}
