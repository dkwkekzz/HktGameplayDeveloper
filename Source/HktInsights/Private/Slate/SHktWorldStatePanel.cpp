// Copyright Hkt Studios, Inc. All Rights Reserved.

#include "Slate/SHktWorldStatePanel.h"
#include "Slate/SHktInsightTable.h"
#include "HktCoreDataCollector.h"
#include "HktInsightsSubsystem.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "HktWorldStatePanel"

void SHktWorldStatePanel::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SVerticalBox)

        // 소스 선택 바
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 4, 0)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("SourceLabel", "Source:"))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&SourceOptions)
                .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Selected, ESelectInfo::Type)
                {
                    if (Selected.IsValid())
                    {
                        SelectedSource = *Selected;
                        RefreshData();
                    }
                })
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                {
                    return SNew(STextBlock).Text(FText::FromString(*Item));
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]() -> FText
                    {
                        return FText::FromString(SelectedSource.IsEmpty() ? TEXT("(auto)") : SelectedSource);
                    })
                ]
            ]
        ]

        // 메인 테이블
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        [
            SAssignNew(Table, SHktInsightTable)
            .Title(LOCTEXT("WorldStateTitle", "WorldState"))
        ]
    ];

    // 초기 데이터 로드 & 서브시스템 바인딩
    RebuildSourceOptions();
    RefreshData();

    // 에디터에서 월드 서브시스템 바인딩 (PIE 시작 시 연결됨)
    // Tick-free: 0.5초 타이머로 소스 목록 갱신 + 데이터 갱신 시도
    // 실제 갱신은 Subsystem::OnDataChanged → Slate Invalidate 흐름으로 처리
    RegisterActiveTimer(0.5f, FWidgetActiveTimerDelegate::CreateLambda(
        [this](double, float) -> EActiveTimerReturnType
        {
            RebuildSourceOptions();

            // 월드 서브시스템에서 이벤트 드리븐 갱신 바인딩 시도
            if (!DataChangedHandle.IsValid())
            {
                if (GEngine)
                {
                    for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
                    {
                        if (UWorld* World = Ctx.World())
                        {
                            if (UHktInsightsWorldSubsystem* Subsystem = World->GetSubsystem<UHktInsightsWorldSubsystem>())
                            {
                                DataChangedHandle = Subsystem->OnDataChanged.AddSP(this, &SHktWorldStatePanel::RefreshData);
                                break;
                            }
                        }
                    }
                }
            }

            return EActiveTimerReturnType::Continue;
        }));
}

void SHktWorldStatePanel::RebuildSourceOptions()
{
    TArray<FString> Categories = FHktCoreDataCollector::Get().GetCategories();
    TArray<FString> WsSources;
    for (const FString& Cat : Categories)
    {
        if (Cat.StartsWith(TEXT("WorldState.")))
        {
            FString Source = Cat.Mid(11); // "WorldState." 이후
            WsSources.AddUnique(Source);
        }
    }

    // 변경 시에만 재구성
    bool bChanged = WsSources.Num() != SourceOptions.Num();
    if (!bChanged)
    {
        for (int32 i = 0; i < WsSources.Num(); ++i)
        {
            if (*SourceOptions[i] != WsSources[i]) { bChanged = true; break; }
        }
    }

    if (bChanged)
    {
        SourceOptions.Reset();
        for (const FString& S : WsSources)
        {
            SourceOptions.Add(MakeShared<FString>(S));
        }
        if (SelectedSource.IsEmpty() && SourceOptions.Num() > 0)
        {
            SelectedSource = *SourceOptions[0];
        }
    }
}

void SHktWorldStatePanel::RefreshData()
{
    if (SelectedSource.IsEmpty())
    {
        // 소스가 없으면 첫 번째 WorldState 카테고리 사용
        RebuildSourceOptions();
        if (SelectedSource.IsEmpty())
        {
            Table->SetData({});
            return;
        }
    }

    const FString Category = FString::Printf(TEXT("WorldState.%s"), *SelectedSource);
    TArray<TPair<FString, FString>> Entries = FHktCoreDataCollector::Get().GetEntries(Category);
    Table->SetData(Entries);
}

#undef LOCTEXT_NAMESPACE
