// Copyright Hkt Studios, Inc. All Rights Reserved.

#include "Slate/SHktVMStatePanel.h"
#include "Slate/SHktInsightTable.h"
#include "HktCoreDataCollector.h"
#include "HktInsightsSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "HktVMStatePanel"

void SHktVMStatePanel::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SAssignNew(Table, SHktInsightTable)
        .Title(LOCTEXT("VMStateTitle", "VM State"))
    ];

    RefreshData();

    // 폴링: 서브시스템 바인딩 시도 + 폴백 갱신
    RegisterActiveTimer(0.5f, FWidgetActiveTimerDelegate::CreateLambda(
        [this](double, float) -> EActiveTimerReturnType
        {
            if (!DataChangedHandle.IsValid())
            {
                if (GEngine)
                {
                    for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
                    {
                        if (UWorld* World = Ctx.World())
                        {
                            if (UHktInsightsWorldSubsystem* Sub = World->GetSubsystem<UHktInsightsWorldSubsystem>())
                            {
                                DataChangedHandle = Sub->OnDataChanged.AddSP(this, &SHktVMStatePanel::RefreshData);
                                break;
                            }
                        }
                    }
                }
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
