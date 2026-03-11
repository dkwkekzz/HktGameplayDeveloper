// Copyright Hkt Studios, Inc. All Rights Reserved.

#include "Slate/SHktInsightTable.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"

#define LOCTEXT_NAMESPACE "HktInsightTable"

void SHktInsightTable::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SVerticalBox)

        // 제목 + 필터
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 8, 0)
            [
                SNew(STextBlock)
                .Text(InArgs._Title)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(SSearchBox)
                .HintText(LOCTEXT("FilterHint", "Filter..."))
                .OnTextChanged_Lambda([this](const FText& Text)
                {
                    FilterText = Text.ToString();
                    RebuildFilteredRows();
                })
            ]
        ]

        // 테이블
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        [
            SAssignNew(ListView, SListView<TSharedPtr<FHktInsightTableRow>>)
            .ListItemsSource(&FilteredRows)
            .OnGenerateRow(this, &SHktInsightTable::OnGenerateRow)
            .SelectionMode(ESelectionMode::Single)
            .HeaderRow
            (
                SNew(SHeaderRow)
                + SHeaderRow::Column("Key")
                .DefaultLabel(LOCTEXT("KeyColumn", "Key"))
                .FillWidth(0.35f)

                + SHeaderRow::Column("Value")
                .DefaultLabel(LOCTEXT("ValueColumn", "Value"))
                .FillWidth(0.65f)
            )
        ]
    ];
}

void SHktInsightTable::SetData(const TArray<TPair<FString, FString>>& InData)
{
    AllRows.Reset(InData.Num());
    for (const auto& Pair : InData)
    {
        TSharedPtr<FHktInsightTableRow> Row = MakeShared<FHktInsightTableRow>();
        Row->Key = Pair.Key;
        Row->Value = Pair.Value;
        AllRows.Add(Row);
    }
    RebuildFilteredRows();
}

void SHktInsightTable::RebuildFilteredRows()
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
            if (Row->Key.Contains(FilterText) || Row->Value.Contains(FilterText))
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

TSharedRef<ITableRow> SHktInsightTable::OnGenerateRow(
    TSharedPtr<FHktInsightTableRow> Item,
    const TSharedRef<STableViewBase>& OwnerTable)
{
    class SInsightTableRow : public SMultiColumnTableRow<TSharedPtr<FHktInsightTableRow>>
    {
    public:
        SLATE_BEGIN_ARGS(SInsightTableRow) {}
        SLATE_END_ARGS()

        void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FHktInsightTableRow> InItem)
        {
            Item = InItem;
            SMultiColumnTableRow::Construct(FSuperRowType::FArguments(), InOwnerTable);
        }

        virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
        {
            if (ColumnName == "Key")
            {
                return SNew(STextBlock)
                    .Text(FText::FromString(Item->Key))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                    .Margin(FMargin(4, 2));
            }
            else
            {
                return SNew(STextBlock)
                    .Text(FText::FromString(Item->Value))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    .Margin(FMargin(4, 2));
            }
        }

        TSharedPtr<FHktInsightTableRow> Item;
    };

    return SNew(SInsightTableRow, OwnerTable, Item);
}

#undef LOCTEXT_NAMESPACE
