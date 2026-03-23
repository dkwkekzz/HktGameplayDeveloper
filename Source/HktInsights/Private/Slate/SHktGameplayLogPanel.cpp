// Copyright Hkt Studios, Inc. All Rights Reserved.

#include "Slate/SHktGameplayLogPanel.h"
#include "HktCoreEventLog.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "HktGameplayLogPanel"

// ── 컬러 팔레트 ──
namespace LogColors
{
    static const FLinearColor Time(0.5f, 0.5f, 0.55f);
    static const FLinearColor Frame(0.6f, 0.6f, 0.65f);
    static const FLinearColor Entity(0.4f, 0.85f, 1.0f);
    static const FLinearColor Message(0.88f, 0.88f, 0.88f);
    static const FLinearColor Dim(0.4f, 0.4f, 0.4f);

    // 카테고리별 색상
    static const FLinearColor CatCoreEntity(0.3f, 0.9f, 0.5f);
    static const FLinearColor CatCoreVM(0.9f, 0.7f, 0.3f);
    static const FLinearColor CatCoreSim(0.3f, 0.7f, 0.9f);
    static const FLinearColor CatCoreStory(0.8f, 0.5f, 0.9f);
    static const FLinearColor CatRuntimeServer(1.0f, 0.5f, 0.3f);
    static const FLinearColor CatRuntimeClient(0.3f, 0.8f, 0.8f);
    static const FLinearColor CatRuntimeIntent(0.9f, 0.9f, 0.3f);
    static const FLinearColor CatPresentation(0.6f, 0.8f, 0.4f);
    static const FLinearColor CatDefault(0.7f, 0.7f, 0.7f);
}

// ============================================================================
// Construct
// ============================================================================

void SHktGameplayLogPanel::Construct(const FArguments& InArgs)
{
    StartTime = FPlatformTime::Seconds();

    ChildSlot
    [
        SNew(SVerticalBox)

        // ── 헤더 바: Active 토글 + Clear + 로그 수 ──
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.f)
        [
            SNew(SHorizontalBox)

            // Active 토글
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 8, 0)
            [
                SNew(SCheckBox)
                .IsChecked_Lambda([]() -> ECheckBoxState
                {
                    return FHktCoreEventLog::Get().IsActive() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                })
                .OnCheckStateChanged_Lambda([](ECheckBoxState NewState)
                {
                    FHktCoreEventLog::Get().SetActive(NewState == ECheckBoxState::Checked);
                })
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("ActiveLabel", "Active"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                ]
            ]

            // Clear 버튼
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 8, 0)
            [
                SNew(SButton)
                .OnClicked_Lambda([this]() -> FReply
                {
                    FHktCoreEventLog::Get().Clear();
                    AllRows.Reset();
                    FilteredRows.Reset();
                    ReadIndex = 0;
                    ListView->RequestListRefresh();
                    return FReply::Handled();
                })
                [ SNew(STextBlock).Text(LOCTEXT("ClearBtn", "Clear")) ]
            ]

            // AutoScroll 토글
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 12, 0)
            [
                SNew(SCheckBox)
                .IsChecked_Lambda([this]() -> ECheckBoxState
                {
                    return bAutoScroll ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                })
                .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
                {
                    bAutoScroll = (NewState == ECheckBoxState::Checked);
                })
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("AutoScrollLabel", "Auto-scroll"))
                ]
            ]

            // 로그 수
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SAssignNew(LogCountText, STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                .ColorAndOpacity(FSlateColor(LogColors::Dim))
            ]
        ]

        // ── 필터 바 ──
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4, 0, 4, 4)
        [
            SNew(SHorizontalBox)

            // 텍스트 검색
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(0, 0, 4, 0)
            [
                SNew(SSearchBox)
                .HintText(LOCTEXT("SearchHint", "Filter messages..."))
                .OnTextChanged_Lambda([this](const FText& Text)
                {
                    FilterText = Text.ToString();
                    RebuildFilteredRows();
                })
            ]

            // Entity ID 필터
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(0, 0, 4, 0)
            [
                SNew(SBox)
                .MinDesiredWidth(150.f)
                [
                    SNew(SSearchBox)
                    .HintText(LOCTEXT("EntityHint", "Entity IDs (1,2,3)"))
                    .OnTextChanged_Lambda([this](const FText& Text)
                    {
                        EntityIdFilterText = Text.ToString();
                        EntityIdFilter.Empty();

                        TArray<FString> Parts;
                        EntityIdFilterText.ParseIntoArray(Parts, TEXT(","), true);
                        for (const FString& Part : Parts)
                        {
                            FString Trimmed = Part.TrimStartAndEnd();
                            if (!Trimmed.IsEmpty())
                            {
                                EntityIdFilter.Add(FCString::Atoi(*Trimmed));
                            }
                        }
                        RebuildFilteredRows();
                    })
                ]
            ]
        ]

        // ── 본문: 카테고리 사이드바 + 로그 리스트 ──
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        [
            SNew(SSplitter)
            .Orientation(Orient_Horizontal)

            // 카테고리 사이드바
            + SSplitter::Slot()
            .Value(0.15f)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    SAssignNew(CategoryContainer, SVerticalBox)
                ]
            ]

            // 로그 리스트
            + SSplitter::Slot()
            .Value(0.85f)
            [
                SAssignNew(ListView, SListView<TSharedPtr<FHktLogEntry>>)
                .ListItemsSource(&FilteredRows)
                .OnGenerateRow(this, &SHktGameplayLogPanel::OnGenerateRow)
                .SelectionMode(ESelectionMode::Single)
                .HeaderRow
                (
                    SNew(SHeaderRow)
                    + SHeaderRow::Column(TEXT("Time"))
                        .DefaultLabel(LOCTEXT("ColTime", "Time"))
                        .FixedWidth(70.f)
                    + SHeaderRow::Column(TEXT("Frame"))
                        .DefaultLabel(LOCTEXT("ColFrame", "Frame"))
                        .FixedWidth(70.f)
                    + SHeaderRow::Column(TEXT("Category"))
                        .DefaultLabel(LOCTEXT("ColCategory", "Category"))
                        .FixedWidth(130.f)
                    + SHeaderRow::Column(TEXT("Entity"))
                        .DefaultLabel(LOCTEXT("ColEntity", "Entity"))
                        .FixedWidth(60.f)
                    + SHeaderRow::Column(TEXT("Tag"))
                        .DefaultLabel(LOCTEXT("ColTag", "Tag"))
                        .FixedWidth(140.f)
                    + SHeaderRow::Column(TEXT("Message"))
                        .DefaultLabel(LOCTEXT("ColMessage", "Message"))
                        .FillWidth(1.0f)
                )
            ]
        ]
    ];

    // 수집 활성화
    FHktCoreEventLog::Get().SetActive(true);

    // 0.1초 폴링
    RegisterActiveTimer(0.1f, FWidgetActiveTimerDelegate::CreateLambda(
        [this](double, float) -> EActiveTimerReturnType
        {
            PollNewEntries();
            return EActiveTimerReturnType::Continue;
        }));
}

SHktGameplayLogPanel::~SHktGameplayLogPanel()
{
    FHktCoreEventLog::Get().SetActive(false);
}

// ============================================================================
// 폴링
// ============================================================================

void SHktGameplayLogPanel::PollNewEntries()
{
    const uint32 CurrentVersion = FHktCoreEventLog::Get().GetVersion();
    if (CurrentVersion == CachedVersion)
    {
        return;
    }
    CachedVersion = CurrentVersion;

    TArray<FHktLogEntry> NewEntries = FHktCoreEventLog::Get().Consume(ReadIndex);
    if (NewEntries.Num() == 0)
    {
        return;
    }

    bool bNewCategory = false;
    for (FHktLogEntry& Entry : NewEntries)
    {
        TSharedPtr<FHktLogEntry> SharedEntry = MakeShared<FHktLogEntry>(MoveTemp(Entry));

        // 새 카테고리 발견 시 활성화
        if (!KnownCategories.HasTagExact(SharedEntry->Category))
        {
            KnownCategories.AddTag(SharedEntry->Category);
            EnabledCategories.AddTag(SharedEntry->Category);
            bNewCategory = true;
        }

        AllRows.Add(SharedEntry);

        if (PassesFilter(*SharedEntry))
        {
            FilteredRows.Add(SharedEntry);
        }
    }

    // 최대 표시 행 제한 (AllRows가 너무 커지지 않도록)
    constexpr int32 MaxDisplayRows = 8192;
    if (AllRows.Num() > MaxDisplayRows)
    {
        const int32 RemoveCount = AllRows.Num() - MaxDisplayRows;
        AllRows.RemoveAt(0, RemoveCount);
        RebuildFilteredRows();
    }

    if (bNewCategory)
    {
        RebuildCategoryCheckboxes();
    }

    LogCountText->SetText(FText::FromString(
        FString::Printf(TEXT("Showing %d / %d entries"), FilteredRows.Num(), AllRows.Num())));

    ListView->RequestListRefresh();

    if (bAutoScroll && FilteredRows.Num() > 0)
    {
        ListView->RequestScrollIntoView(FilteredRows.Last());
    }
}

// ============================================================================
// 필터
// ============================================================================

bool SHktGameplayLogPanel::PassesFilter(const FHktLogEntry& Entry) const
{
    // 카테고리 필터: HasTag()는 계층적 매칭 (부모 태그 활성화 시 자식도 통과)
    if (!EnabledCategories.HasTagExact(Entry.Category))
    {
        return false;
    }

    // Entity ID 필터
    if (EntityIdFilter.Num() > 0)
    {
        if (Entry.EntityId == InvalidEntityId || !EntityIdFilter.Contains(Entry.EntityId))
        {
            return false;
        }
    }

    // 텍스트 검색
    if (!FilterText.IsEmpty())
    {
        const FString CategoryDisplay = GetCategoryDisplayName(Entry.Category);
        if (!Entry.Message.Contains(FilterText) &&
            !CategoryDisplay.Contains(FilterText) &&
            !Entry.EventTag.ToString().Contains(FilterText))
        {
            return false;
        }
    }

    return true;
}

void SHktGameplayLogPanel::RebuildFilteredRows()
{
    FilteredRows.Reset();
    for (const auto& Row : AllRows)
    {
        if (PassesFilter(*Row))
        {
            FilteredRows.Add(Row);
        }
    }

    LogCountText->SetText(FText::FromString(
        FString::Printf(TEXT("Showing %d / %d entries"), FilteredRows.Num(), AllRows.Num())));

    ListView->RequestListRefresh();
}

void SHktGameplayLogPanel::RebuildCategoryCheckboxes()
{
    CategoryContainer->ClearChildren();

    // 헤더
    CategoryContainer->AddSlot()
    .AutoHeight()
    .Padding(4, 2)
    [
        SNew(STextBlock)
        .Text(LOCTEXT("CatHeader", "Categories"))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
        .ColorAndOpacity(FSlateColor(LogColors::Message))
    ];

    // 전체 선택/해제
    CategoryContainer->AddSlot()
    .AutoHeight()
    .Padding(4, 1)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0, 0, 4, 0)
        [
            SNew(SButton)
            .OnClicked_Lambda([this]() -> FReply
            {
                EnabledCategories = KnownCategories;
                RebuildCategoryCheckboxes();
                RebuildFilteredRows();
                return FReply::Handled();
            })
            [ SNew(STextBlock).Text(LOCTEXT("AllBtn", "All")) ]
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(SButton)
            .OnClicked_Lambda([this]() -> FReply
            {
                EnabledCategories.Reset();
                RebuildCategoryCheckboxes();
                RebuildFilteredRows();
                return FReply::Handled();
            })
            [ SNew(STextBlock).Text(LOCTEXT("NoneBtn", "None")) ]
        ]
    ];

    // 카테고리별 체크박스 (정렬)
    TArray<FGameplayTag> Tags;
    KnownCategories.GetGameplayTagArray(Tags);
    Tags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
    {
        return A.GetTagName().LexicalLess(B.GetTagName());
    });

    for (const FGameplayTag& CatTag : Tags)
    {
        FGameplayTag CapturedTag = CatTag;
        FString DisplayName = GetCategoryDisplayName(CatTag);

        CategoryContainer->AddSlot()
        .AutoHeight()
        .Padding(4, 1)
        [
            SNew(SCheckBox)
            .IsChecked_Lambda([this, CapturedTag]() -> ECheckBoxState
            {
                return EnabledCategories.HasTagExact(CapturedTag) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
            })
            .OnCheckStateChanged_Lambda([this, CapturedTag](ECheckBoxState NewState)
            {
                if (NewState == ECheckBoxState::Checked)
                {
                    EnabledCategories.AddTag(CapturedTag);
                }
                else
                {
                    EnabledCategories.RemoveTag(CapturedTag);
                }
                RebuildFilteredRows();
            })
            [
                SNew(STextBlock)
                .Text(FText::FromString(DisplayName))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                .ColorAndOpacity(GetCategoryColor(CapturedTag))
            ]
        ];
    }
}

// ============================================================================
// 행 생성
// ============================================================================

TSharedRef<ITableRow> SHktGameplayLogPanel::OnGenerateRow(
    TSharedPtr<FHktLogEntry> Item,
    const TSharedRef<STableViewBase>& OwnerTable)
{
    class SLogRow : public SMultiColumnTableRow<TSharedPtr<FHktLogEntry>>
    {
    public:
        SLATE_BEGIN_ARGS(SLogRow) {}
        SLATE_END_ARGS()

        void Construct(const FArguments& InArgs,
                       const TSharedRef<STableViewBase>& InOwnerTable,
                       TSharedPtr<FHktLogEntry> InItem,
                       double InStartTime)
        {
            Item = InItem;
            PanelStartTime = InStartTime;
            SMultiColumnTableRow::Construct(FSuperRowType::FArguments(), InOwnerTable);
        }

        virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
        {
            FString ColStr = ColumnName.ToString();

            if (ColStr == TEXT("Time"))
            {
                double RelTime = Item->Timestamp - PanelStartTime;
                return SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("%.2fs"), RelTime)))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                    .ColorAndOpacity(FSlateColor(LogColors::Time))
                    .Margin(FMargin(2, 1));
            }

            if (ColStr == TEXT("Frame"))
            {
                return SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("%llu"), Item->FrameNumber)))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                    .ColorAndOpacity(FSlateColor(LogColors::Frame))
                    .Margin(FMargin(2, 1));
            }

            if (ColStr == TEXT("Category"))
            {
                FString DisplayName = SHktGameplayLogPanel::GetCategoryDisplayName(Item->Category);
                return SNew(STextBlock)
                    .Text(FText::FromString(DisplayName))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                    .ColorAndOpacity(SHktGameplayLogPanel::GetCategoryColor(Item->Category))
                    .Margin(FMargin(2, 1));
            }

            if (ColStr == TEXT("Entity"))
            {
                FString EntityStr = Item->EntityId != InvalidEntityId
                    ? FString::Printf(TEXT("%d"), Item->EntityId)
                    : TEXT("-");
                return SNew(STextBlock)
                    .Text(FText::FromString(EntityStr))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                    .ColorAndOpacity(FSlateColor(LogColors::Entity))
                    .Margin(FMargin(2, 1));
            }

            if (ColStr == TEXT("Tag"))
            {
                FString TagStr = Item->EventTag.IsValid() ? Item->EventTag.ToString() : TEXT("-");
                return SNew(STextBlock)
                    .Text(FText::FromString(TagStr))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                    .ColorAndOpacity(FSlateColor(LogColors::Dim))
                    .Margin(FMargin(2, 1));
            }

            // Message
            return SNew(STextBlock)
                .Text(FText::FromString(Item->Message))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                .ColorAndOpacity(FSlateColor(LogColors::Message))
                .Margin(FMargin(2, 1));
        }

        TSharedPtr<FHktLogEntry> Item;
        double PanelStartTime = 0.0;
    };

    return SNew(SLogRow, OwnerTable, Item, StartTime);
}

// ============================================================================
// 카테고리 색상 — MatchesTag()로 계층적 매칭
// ============================================================================

FSlateColor SHktGameplayLogPanel::GetCategoryColor(const FGameplayTag& Category)
{
    if (Category.MatchesTag(HktLogTags::Core_Entity))       return FSlateColor(LogColors::CatCoreEntity);
    if (Category.MatchesTag(HktLogTags::Core_VM))           return FSlateColor(LogColors::CatCoreVM);
    if (Category.MatchesTag(HktLogTags::Core_Story))        return FSlateColor(LogColors::CatCoreStory);
    if (Category.MatchesTag(HktLogTags::Runtime_Server))    return FSlateColor(LogColors::CatRuntimeServer);
    if (Category.MatchesTag(HktLogTags::Runtime_Client))    return FSlateColor(LogColors::CatRuntimeClient);
    if (Category.MatchesTag(HktLogTags::Runtime_Intent))    return FSlateColor(LogColors::CatRuntimeIntent);
    if (Category.MatchesTag(HktLogTags::Presentation))      return FSlateColor(LogColors::CatPresentation);
    return FSlateColor(LogColors::CatDefault);
}

// ============================================================================
// 표시명: "HktLog." 접두사 제거
// ============================================================================

FString SHktGameplayLogPanel::GetCategoryDisplayName(const FGameplayTag& Category)
{
    static const FString Prefix = TEXT("HktLog.");
    FString TagStr = Category.ToString();
    if (TagStr.StartsWith(Prefix))
    {
        TagStr.RightChopInline(Prefix.Len());
    }
    return TagStr;
}

#undef LOCTEXT_NAMESPACE
