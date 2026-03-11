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
                        CachedVersion = 0;
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

        // ── 상단: 엔티티 목록 (고정 3컬럼) ──
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        [
            SAssignNew(ListView, SListView<TSharedPtr<FHktWorldStateEntityRow>>)
            .ListItemsSource(&FilteredRows)
            .OnGenerateRow(this, &SHktWorldStatePanel::OnGenerateRow)
            .OnSelectionChanged_Lambda([this](TSharedPtr<FHktWorldStateEntityRow> Item, ESelectInfo::Type SelectInfo)
            {
                if (SelectInfo == ESelectInfo::Direct) return;  // 코드에서 호출한 SetSelection 무시
                SelectedEntityKey = Item.IsValid() ? Item->EntityKey : FString();
                BuildDetailPanel();
            })
            .SelectionMode(ESelectionMode::Single)
            .HeaderRow
            (
                SNew(SHeaderRow)
                + SHeaderRow::Column(TEXT("_Entity"))
                    .DefaultLabel(LOCTEXT("ColEntity", "Entity"))
                    .FillWidth(1.0f)
                + SHeaderRow::Column(TEXT("Type"))
                    .DefaultLabel(LOCTEXT("ColType", "Type"))
                    .FillWidth(1.0f)
                + SHeaderRow::Column(TEXT("Owner"))
                    .DefaultLabel(LOCTEXT("ColOwner", "Owner"))
                    .FillWidth(1.0f)
            )
        ]

        // ── 하단: 선택 상세 패널 ──
        + SVerticalBox::Slot()
        .FillHeight(0.6f)
        .Padding(4.f)
        [
            SAssignNew(DetailContainer, SVerticalBox)
        ]
    ];

    CachedVersion = FHktCoreDataCollector::Get().GetVersion();
    RebuildSourceOptions();
    RefreshData();
    BuildDetailPanel();

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
            EntityRowMap.Empty();
            ListView->RequestListRefresh();
            FrameText->SetText(FText::GetEmpty());
            EntityCountText->SetText(FText::GetEmpty());
            return;
        }
    }

    const FString Category = FString::Printf(TEXT("WorldState.%s"), *SelectedSource);
    TArray<TPair<FString, FString>> Entries = FHktCoreDataCollector::Get().GetEntries(Category);

    // ── 파싱 ──
    FString FrameStr, CountStr;
    TMap<FString, TMap<FString, FString>> ParsedEntities;
    TSet<FString> SeenProps;
    TArray<FString> NewPropertyOrder;

    // Type, Owner 선두 고정
    NewPropertyOrder.Add(TEXT("Type"));  SeenProps.Add(TEXT("Type"));
    NewPropertyOrder.Add(TEXT("Owner")); SeenProps.Add(TEXT("Owner"));

    for (const auto& Entry : Entries)
    {
        if (Entry.Key == TEXT("_Frame"))       { FrameStr = Entry.Value; continue; }
        if (Entry.Key == TEXT("_EntityCount")) { CountStr = Entry.Value; continue; }
        if (!Entry.Key.StartsWith(TEXT("E_"))) continue;

        TMap<FString, FString>& Props = ParsedEntities.Add(Entry.Key);
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
                Props.Add(PropName, PropValue);

                if (!SeenProps.Contains(PropName))
                {
                    SeenProps.Add(PropName);
                    NewPropertyOrder.Add(PropName);
                }
            }
        }
    }

    // ── 메타 갱신 ──
    FrameText->SetText(FText::FromString(
        FrameStr.IsEmpty() ? TEXT("") : FString::Printf(TEXT("Frame: %s"), *FrameStr)));
    EntityCountText->SetText(FText::FromString(
        CountStr.IsEmpty() ? TEXT("") : FString::Printf(TEXT("  Entities: %s"), *CountStr)));

    // ── 엔티티 셋 변경 감지 & Props in-place 갱신 ──
    bool bEntitySetChanged = false;

    // 제거된 엔티티
    TArray<FString> ToRemove;
    for (auto& KV : EntityRowMap)
    {
        if (!ParsedEntities.Contains(KV.Key))
        {
            ToRemove.Add(KV.Key);
            bEntitySetChanged = true;
        }
    }
    for (const FString& Key : ToRemove)
    {
        EntityRowMap.Remove(Key);
    }

    // 추가 & 기존 Props 갱신
    for (auto& KV : ParsedEntities)
    {
        if (TSharedPtr<FHktWorldStateEntityRow>* Existing = EntityRowMap.Find(KV.Key))
        {
            // 기존 엔티티: Props만 갱신 (포인터 안정, 람다가 자동으로 최신 값 읽음)
            (*Existing)->Props = MoveTemp(KV.Value);
        }
        else
        {
            // 신규 엔티티
            TSharedPtr<FHktWorldStateEntityRow> Row = MakeShared<FHktWorldStateEntityRow>();
            Row->EntityKey = KV.Key;
            Row->Props = MoveTemp(KV.Value);
            EntityRowMap.Add(KV.Key, Row);
            bEntitySetChanged = true;
        }
    }

    // ── 상단 리스트: 엔티티 추가/삭제 시에만 갱신 ──
    if (bEntitySetChanged)
    {
        AllRows.Reset();
        for (auto& KV : EntityRowMap)
        {
            AllRows.Add(KV.Value);
        }
        AllRows.Sort([](const TSharedPtr<FHktWorldStateEntityRow>& A,
                        const TSharedPtr<FHktWorldStateEntityRow>& B)
        {
            // E_0, E_1, ... 숫자 순 정렬
            int32 IdA = FCString::Atoi(*A->EntityKey.Mid(2));
            int32 IdB = FCString::Atoi(*B->EntityKey.Mid(2));
            return IdA < IdB;
        });

        RebuildFilteredRows();

        // 선택 복원
        if (!SelectedEntityKey.IsEmpty())
        {
            if (TSharedPtr<FHktWorldStateEntityRow>* SelRow = EntityRowMap.Find(SelectedEntityKey))
            {
                ListView->SetSelection(*SelRow, ESelectInfo::Direct);
            }
            else
            {
                // 선택된 엔티티가 제거됨
                SelectedEntityKey.Empty();
                BuildDetailPanel();
            }
        }
    }

    // ── 하단 상세: 프로퍼티 구성 변경 시에만 위젯 재구성 ──
    AllPropertyNames = NewPropertyOrder;

    if (!SelectedEntityKey.IsEmpty() && DetailBuiltProps != AllPropertyNames)
    {
        BuildDetailPanel();
    }
    // (프로퍼티 구성 동일 → 위젯 그대로, 람다가 최신 Props 읽음 → 깜빡임 없음)
}

// ============================================================================
// Entity List (상단)
// ============================================================================

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
                    if (Prop.Value.Contains(FilterText))
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

    ListView->RequestListRefresh();
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
// Detail Panel (하단) — 위젯 1회 생성, 값만 실시간 갱신
// ============================================================================

void SHktWorldStatePanel::BuildDetailPanel()
{
    DetailContainer->ClearChildren();
    DetailBuiltProps.Reset();

    if (SelectedEntityKey.IsEmpty())
    {
        DetailContainer->AddSlot()
        .AutoHeight()
        .Padding(4.f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("NoSelection", "Select an entity to view details"))
            .ColorAndOpacity(FSlateColor(WSColors::Dim))
            .Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
        ];
        return;
    }

    // 선택된 엔티티 Row 찾기 (안정적 포인터)
    TSharedPtr<FHktWorldStateEntityRow> Entity;
    if (TSharedPtr<FHktWorldStateEntityRow>* Found = EntityRowMap.Find(SelectedEntityKey))
    {
        Entity = *Found;
    }
    if (!Entity.IsValid()) return;

    // 현재 프로퍼티 구성 기록
    DetailBuiltProps = AllPropertyNames;

    // ── 헤더: Entity (Type) — TAttribute 람다로 Type 실시간 반영 ──
    TWeakPtr<FHktWorldStateEntityRow> WeakEntity = Entity;

    DetailContainer->AddSlot()
    .AutoHeight()
    .Padding(4, 2, 4, 4)
    [
        SNew(STextBlock)
        .Text(TAttribute<FText>::CreateLambda([WeakEntity]()
        {
            if (auto E = WeakEntity.Pin())
            {
                FString TypeStr = TEXT("?");
                if (const FString* T = E->Props.Find(TEXT("Type"))) TypeStr = *T;
                return FText::FromString(FString::Printf(TEXT("%s  (%s)"), *E->EntityKey, *TypeStr));
            }
            return FText::GetEmpty();
        }))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
        .ColorAndOpacity(FSlateColor(WSColors::Key))
    ];

    // ── 프로퍼티 목록 — 위젯 1회 생성, 값은 TAttribute 람다 ──
    TSharedRef<SScrollBox> PropsScroll = SNew(SScrollBox);

    for (const FString& PropName : AllPropertyNames)
    {
        FString CapturedProp = PropName;  // 람다 캡처용 복사

        PropsScroll->AddSlot()
        .Padding(4, 1)
        [
            SNew(SHorizontalBox)

            // 프로퍼티 이름 (고정)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SBox)
                .MinDesiredWidth(120.f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(PropName))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    .ColorAndOpacity(FSlateColor(WSColors::FieldName))
                ]
            ]

            // 프로퍼티 값 (TAttribute 람다 — Props 변경 시 자동 반영)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(STextBlock)
                .Text(TAttribute<FText>::CreateLambda([WeakEntity, CapturedProp]()
                {
                    if (auto E = WeakEntity.Pin())
                    {
                        if (const FString* V = E->Props.Find(CapturedProp))
                            return FText::FromString(*V);
                    }
                    return FText::FromString(TEXT("-"));
                }))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                .ColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([WeakEntity, CapturedProp]()
                {
                    if (auto E = WeakEntity.Pin())
                    {
                        if (const FString* V = E->Props.Find(CapturedProp))
                            return GetPropColor(CapturedProp, *V);
                    }
                    return FSlateColor(WSColors::Dim);
                }))
            ]
        ];
    }

    DetailContainer->AddSlot()
    .FillHeight(1.0f)
    .Padding(2.f)
    [
        PropsScroll
    ];
}

#undef LOCTEXT_NAMESPACE
