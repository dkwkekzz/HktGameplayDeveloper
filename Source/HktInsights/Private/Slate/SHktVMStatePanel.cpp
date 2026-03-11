// Copyright Hkt Studios, Inc. All Rights Reserved.

#include "Slate/SHktVMStatePanel.h"
#include "Slate/SHktInsightTable.h"
#include "HktCoreDataCollector.h"

#define LOCTEXT_NAMESPACE "HktVMStatePanel"

void SHktVMStatePanel::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SAssignNew(Table, SHktInsightTable)
        .Title(LOCTEXT("VMStateTitle", "VM State"))
    ];

    CachedVersion = FHktCoreDataCollector::Get().GetVersion();
    RefreshData();

    // 0.1초 간격으로 Version 직접 폴링 — 서브시스템 의존 없음
    RegisterActiveTimer(0.1f, FWidgetActiveTimerDelegate::CreateLambda(
        [this](double, float) -> EActiveTimerReturnType
        {
            const uint32 CurrentVersion = FHktCoreDataCollector::Get().GetVersion();
            if (CurrentVersion != CachedVersion)
            {
                CachedVersion = CurrentVersion;
                RefreshData();
            }
            return EActiveTimerReturnType::Continue;
        }));
}

void SHktVMStatePanel::RefreshData()
{
    TArray<TPair<FString, FString>> Entries = FHktCoreDataCollector::Get().GetEntries(TEXT("VM"));
    Table->SetData(Entries);
}

#undef LOCTEXT_NAMESPACE
