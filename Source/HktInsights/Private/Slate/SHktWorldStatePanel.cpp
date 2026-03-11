// Copyright Hkt Studios, Inc. All Rights Reserved.

#include "Slate/SHktWorldStatePanel.h"
#include "HktCoreDataCollector.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "HktWorldStatePanel"

// ── 컬러 팔레트 ──
namespace WSColors
{
    static const FLinearColor Key(0.4f, 0.85f, 1.0f);
    static const FLinearColor Header(0.7f, 0.7f, 0.75f);
    static const FLinearColor Value(0.88f, 0.88f, 0.88f);
    static const FLinearColor FieldName(0.5f, 0.5f, 0.58f);
    static const FLinearColor Dim(0.4f, 0.4f, 0.4f);
    static const FLinearColor TypeUnit(0.3f, 0.8f, 1.0f);
    static const FLinearColor TypeBuilding(1.0f, 0.7f, 0.3f);
    static const FLinearColor TypeProjectile(1.0f, 0.4f, 0.4f);
    static const FLinearColor TypeEquipment(0.7f, 0.5f, 1.0f);
}

static FSlateColor GetPropColor(const FString& PropName, const FString& Value)
{
    if (PropName == TEXT("Type"))
    {
        if (Value == TEXT("Unit"))        return FSlateColor(WSColors::TypeUnit);
        if (Value == TEXT("Building"))    return FSlateColor(WSColors::TypeBuilding);
        if (Value == TEXT("Projectile")) return FSlateColor(WSColors::TypeProjectile);
        if (Value == TEXT("Equipment"))  return FSlateColor(WSColors::TypeEquipment);
    }
    return FSlateColor(WSColors::Value);
}

// ============================================================================
// Construct
// ============================================================================

void SHktWorldStatePanel::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SVerticalBox)

        // ── 헤더 바: Source + Meta ──
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
                .ColorAndOpacity(FSlateColor(WSColors::FieldName))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(0, 0, 12, 0)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&SourceOptions)
                .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Selected, ESelectInfo::Type)
                {
                    if (Selected.IsValid())
                    {
                        SelectedSource = *Selected;
                        CachedVersion = 0; // 강제 갱신
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
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 8, 0)
            [
                SAssignNew(FrameText, STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                .ColorAndOpacity(FSlateColor(WSColors::Key))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SAssignNew(EntityCountText, STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                .ColorAndOpacity(FSlateColor(WSColors::Header))
            ]
        ]

        // ── 필터 바 ──
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4, 0, 4, 4)
        [
            SNew(SSearchBox)
            .HintText(LOCTEXT("FilterHint", "Filter entities..."))
            .OnTextChanged_Lambda([this](const FText& Text)
            {
                FilterText = Text.ToString();
                RebuildFilteredRows();
            })
        ]

        // ── 엔티티 그리드 ──
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        [
            SAssignNew(GridContainer, SVerticalBox)
        ]

        // ── 선택 상세 패널 ──
        + SVerticalBox::Slot()
        .AutoHeight()
        .MaxHeight(200.f)
        .Padding(4.f)
        [
            SAssignNew(DetailContainer, SVerticalBox)
        ]
    ];

    CachedVersion = FHktCoreDataCollector::Get().GetVersion();
    RebuildSourceOptions();
    RefreshData();

    // 0.1초 Version 폴링
    RegisterActiveTimer(0.1f, FWidgetActiveTimerDelegate::CreateLambda(
        [this](double, float) -> EActiveTimerReturnType
        {
            const uint32 CurrentVersion = FHktCoreDataCollector::Get().GetVersion();
            if (CurrentVersion != CachedVersion)
            {
                CachedVersion = CurrentVersion;
                RebuildSourceOptions();
                RefreshData();
            }
            return EActiveTimerReturnType::Continue;
        }));
}

// ============================================================================
// Source Options
// ============================================================================

void SHktWorldStatePanel::RebuildSourceOptions()
{
    TArray<FString> Categories = FHktCoreDataCollector::Get().GetCategories();
    TArray<FString> WsSources;
    for (const FString& Cat : Categories)
    {
        if (Cat.StartsWith(TEXT("WorldState.")))
        {
            WsSources.AddUnique(Cat.Mid(11));
        }
    }

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

// ============================================================================
// Data Refresh
// ============================================================================

void SHktWorldStatePanel::RefreshData()
{
    if (SelectedSource.IsEmpty())
    {
        RebuildSourceOptions();
        if (SelectedSource.IsEmpty())
        {
            AllRows.Reset();
            FilteredRows.Reset();
            if (ListView.IsValid()) ListView->RequestListRefresh();
            FrameText->SetText(FText::GetEmpty());
            EntityCountText->SetText(FText::GetEmpty());
            return;
        }
    }

    const FString Category = FString::Printf(TEXT("WorldState.%s"), *SelectedSource);
    TArray<TPair<FString, FString>> Entries = FHktCoreDataCollector::Get().GetEntries(Category);

    // 메타 vs 엔티티 분리 & 파싱
    FString FrameStr, CountStr;
    TArray<TSharedPtr<FHktWorldStateEntityRow>> NewRows;
    TSet<FString> SeenColumns;
    TArray<FString> NewColumnOrder;

    // Type, Owner 고정 선두 컬럼
    NewColumnOrder.Add(TEXT("Type"));  SeenColumns.Add(TEXT("Type"));
    NewColumnOrder.Add(TEXT("Owner")); SeenColumns.Add(TEXT("Owner"));

    for (const auto& Entry : Entries)
    {
        if (Entry.Key == TEXT("_Frame"))       { FrameStr = Entry.Value; continue; }
        if (Entry.Key == TEXT("_EntityCount")) { CountStr = Entry.Value; continue; }
        if (!Entry.Key.StartsWith(TEXT("E_"))) continue;

        TSharedPtr<FHktWorldStateEntityRow> Row = MakeShared<FHktWorldStateEntityRow>();
        Row->EntityKey = Entry.Key;

        TArray<FString> Segments;
        Entry.Value.ParseIntoArray(Segments, TEXT("|"), true);
        for (FString& Seg : Segments)
        {
            Seg.TrimStartAndEndInline();
            int32 EqIdx;
            if (Seg.FindChar(TEXT('='), EqIdx) && EqIdx > 0)
            {
                FString PropName = Seg.Left(EqIdx);
                FString PropValue = Seg.Mid(EqIdx + 1);
                Row->Props.Add(PropName, PropValue);

                if (!SeenColumns.Contains(PropName))
                {
                    SeenColumns.Add(PropName);
                    NewColumnOrder.Add(PropName);
                }
            }
        }
        NewRows.Add(Row);
    }

    // 메타 정보 갱신
    FrameText->SetText(FText::FromString(
        FrameStr.IsEmpty() ? TEXT("") : FString::Printf(TEXT("Frame: %s"), *FrameStr)));
    EntityCountText->SetText(FText::FromString(
        CountStr.IsEmpty() ? TEXT("") : FString::Printf(TEXT("  Entities: %s"), *CountStr)));

    AllRows = MoveTemp(NewRows);

    // 컬럼 변경 시 그리드 재구성
    if (NewColumnOrder != CurrentColumns)
    {
        CurrentColumns = NewColumnOrder;
        RebuildGrid();
    }

    RebuildFilteredRows();

    // 선택 상태 복원
    if (SelectedEntity.IsValid() && ListView.IsValid())
    {
        FString SelKey = SelectedEntity->EntityKey;
        TSharedPtr<FHktWorldStateEntityRow> NewSel;
        for (const auto& Row : FilteredRows)
        {
            if (Row->EntityKey == SelKey) { NewSel = Row; break; }
        }
        SelectedEntity = NewSel;
        if (NewSel.IsValid())
        {
            ListView->SetSelection(NewSel, ESelectInfo::Direct);
        }
        else
        {
            ListView->ClearSelection();
        }
    }

    UpdateDetailPanel();
}

// ============================================================================
// Grid (SListView + SMultiColumnTableRow)
// ============================================================================

void SHktWorldStatePanel::RebuildGrid()
{
    GridContainer->ClearChildren();

    HeaderRow = SNew(SHeaderRow);

    // Entity ID 컬럼 (고정 선두)
    HeaderRow->AddColumn(
        SHeaderRow::Column(TEXT("_Entity"))
        .DefaultLabel(FText::FromString(TEXT("Entity")))
        .FillWidth(1.2f)
    );

    // 프로퍼티 컬럼
    for (const FString& Col : CurrentColumns)
    {
        float Width = (Col == TEXT("Type")) ? 1.2f : 1.0f;
        HeaderRow->AddColumn(
            SHeaderRow::Column(FName(*Col))
            .DefaultLabel(FText::FromString(Col))
            .FillWidth(Width)
        );
    }

    GridContainer->AddSlot()
    .FillHeight(1.0f)
    [
        SAssignNew(ListView, SListView<TSharedPtr<FHktWorldStateEntityRow>>)
        .ListItemsSource(&FilteredRows)
        .OnGenerateRow(this, &SHktWorldStatePanel::OnGenerateRow)
        .OnSelectionChanged_Lambda([this](TSharedPtr<FHktWorldStateEntityRow> Item, ESelectInfo::Type)
        {
            SelectedEntity = Item;
            UpdateDetailPanel();
        })
        .SelectionMode(ESelectionMode::Single)
        .HeaderRow(HeaderRow.ToSharedRef())
    ];
}

void SHktWorldStatePanel::RebuildFilteredRows()
{
    FilteredRows.Reset();

    if (FilterText.IsEmpty())
    {
        FilteredRows = AllRows;
    }
    else
    {
        for (const auto& Row : AllRows)
        {
            bool bMatch = Row->EntityKey.Contains(FilterText);
            if (!bMatch)
            {
                for (const auto& Prop : Row->Props)
                {
                    if (Prop.Key.Contains(FilterText) || Prop.Value.Contains(FilterText))
                    {
                        bMatch = true;
                        break;
                    }
                }
            }
            if (bMatch)
            {
                FilteredRows.Add(Row);
            }
        }
    }

    if (ListView.IsValid())
    {
        ListView->RequestListRefresh();
    }
}

TSharedRef<ITableRow> SHktWorldStatePanel::OnGenerateRow(
    TSharedPtr<FHktWorldStateEntityRow> Item,
    const TSharedRef<STableViewBase>& OwnerTable)
{
    class SEntityRow : public SMultiColumnTableRow<TSharedPtr<FHktWorldStateEntityRow>>
    {
    public:
        SLATE_BEGIN_ARGS(SEntityRow) {}
        SLATE_END_ARGS()

        void Construct(const FArguments& InArgs,
                       const TSharedRef<STableViewBase>& InOwnerTable,
                       TSharedPtr<FHktWorldStateEntityRow> InItem)
        {
            Item = InItem;
            SMultiColumnTableRow::Construct(FSuperRowType::FArguments(), InOwnerTable);
        }

        virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
        {
            FString ColStr = ColumnName.ToString();

            if (ColStr == TEXT("_Entity"))
            {
                return SNew(STextBlock)
                    .Text(FText::FromString(Item->EntityKey))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                    .ColorAndOpacity(FSlateColor(WSColors::Key))
                    .Margin(FMargin(4, 1));
            }

            if (const FString* Val = Item->Props.Find(ColStr))
            {
                return SNew(STextBlock)
                    .Text(FText::FromString(*Val))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    .ColorAndOpacity(GetPropColor(ColStr, *Val))
                    .Margin(FMargin(4, 1));
            }

            return SNew(STextBlock)
                .Text(FText::FromString(TEXT("-")))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                .ColorAndOpacity(FSlateColor(WSColors::Dim))
                .Margin(FMargin(4, 1));
        }

        TSharedPtr<FHktWorldStateEntityRow> Item;
    };

    return SNew(SEntityRow, OwnerTable, Item);
}

// ============================================================================
// Detail Panel
// ============================================================================

void SHktWorldStatePanel::UpdateDetailPanel()
{
    DetailContainer->ClearChildren();

    if (!SelectedEntity.IsValid())
    {
        DetailContainer->AddSlot()
        .AutoHeight()
        .Padding(2.f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("NoSelection", "Select an entity to view details"))
            .ColorAndOpacity(FSlateColor(WSColors::Dim))
            .Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
        ];
        return;
    }

    // 헤더: Entity + Type
    FString TypeStr = TEXT("Unknown");
    if (const FString* T = SelectedEntity->Props.Find(TEXT("Type")))
    {
        TypeStr = *T;
    }

    DetailContainer->AddSlot()
    .AutoHeight()
    .Padding(2, 2, 2, 4)
    [
        SNew(STextBlock)
        .Text(FText::FromString(FString::Printf(TEXT("%s  (%s)"), *SelectedEntity->EntityKey, *TypeStr)))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
        .ColorAndOpacity(FSlateColor(WSColors::Key))
    ];

    // 프로퍼티 — 세로 나열 (스크롤 가능)
    TSharedRef<SScrollBox> PropsScroll = SNew(SScrollBox);

    for (const FString& Col : CurrentColumns)
    {
        if (const FString* Val = SelectedEntity->Props.Find(Col))
        {
            PropsScroll->AddSlot()
            .Padding(2, 1)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SBox)
                    .MinDesiredWidth(100.f)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(Col))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                        .ColorAndOpacity(FSlateColor(WSColors::FieldName))
                    ]
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(*Val))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                    .ColorAndOpacity(GetPropColor(Col, *Val))
                ]
            ];
        }
    }

    DetailContainer->AddSlot()
    .FillHeight(1.0f)
    .Padding(2.f)
    [
        PropsScroll
    ];
}

#undef LOCTEXT_NAMESPACE
