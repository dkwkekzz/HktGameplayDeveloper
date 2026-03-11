// Copyright Hkt Studios, Inc. All Rights Reserved.

#include "Slate/SHktRuntimeInsightsPanel.h"
#include "Slate/SHktInsightTable.h"
#include "HktCoreDataCollector.h"
#include "HktInsightsSubsystem.h"
#include "Widgets/Text/STextBlock.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "HktRuntimeInsightsPanel"

void SHktRuntimeInsightsPanel::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("RuntimeTitle", "Runtime Insights"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
        ]
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        [
            SAssignNew(TabContainer, SVerticalBox)
        ]
    ];

    RebuildTables();
    RefreshData();

    // 폴링: 서브시스템 바인딩 + 카테고리 변경 감지
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
                                DataChangedHandle = Sub->OnDataChanged.AddSP(this, &SHktRuntimeInsightsPanel::RefreshData);
                                break;
                            }
                        }
                    }
                }
            }

            // 카테고리 변경 시 테이블 재구성
            RebuildTables();
            return EActiveTimerReturnType::Continue;
        }));
}

void SHktRuntimeInsightsPanel::RebuildTables()
{
    TArray<FString> AllCategories = FHktCoreDataCollector::Get().GetCategories();
    TArray<FString> RuntimeCategories;
    for (const FString& Cat : AllCategories)
    {
        if (Cat.StartsWith(TEXT("Runtime.")))
        {
            RuntimeCategories.Add(Cat);
        }
    }
    RuntimeCategories.Sort();

    // 카테고리 변경 여부 확인
    bool bChanged = RuntimeCategories.Num() != Tabs.Num();
    if (!bChanged)
    {
        for (int32 i = 0; i < RuntimeCategories.Num(); ++i)
        {
            if (Tabs[i].Category != RuntimeCategories[i]) { bChanged = true; break; }
        }
    }

    if (!bChanged) return;

    // 컨테이너 초기화 및 재구성
    TabContainer->ClearChildren();
    Tabs.Reset();

    for (const FString& Cat : RuntimeCategories)
    {
        FString ShortName = Cat.Mid(8); // "Runtime." 이후

        FRuntimeTab Tab;
        Tab.Category = Cat;

        TabContainer->AddSlot()
        .FillHeight(1.0f)
        .Padding(2.f)
        [
            SAssignNew(Tab.Table, SHktInsightTable)
            .Title(FText::FromString(ShortName))
        ];

        Tabs.Add(MoveTemp(Tab));
    }
}

void SHktRuntimeInsightsPanel::RefreshData()
{
    RebuildTables();

    for (FRuntimeTab& Tab : Tabs)
    {
        TArray<TPair<FString, FString>> Entries = FHktCoreDataCollector::Get().GetEntries(Tab.Category);
        Tab.Table->SetData(Entries);
    }
}

#undef LOCTEXT_NAMESPACE
