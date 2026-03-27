// Copyright Hkt Studios, Inc. All Rights Reserved.

#include "Slate/SHktVMStatePanel.h"
#include "HktCoreDataCollector.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "HktVMStatePanel"

// ── 컬러 팔레트 ──
namespace VMColors
{
    static const FLinearColor EntityKey(0.4f, 0.85f, 1.0f);      // Cyan
    static const FLinearColor EntityDim(0.55f, 0.7f, 0.8f);      // Dim cyan (VM 없는 entity)
    static const FLinearColor VMKey(0.85f, 0.7f, 1.0f);          // Light purple
    static const FLinearColor Header(0.7f, 0.7f, 0.75f);         // Gray
    static const FLinearColor Value(0.88f, 0.88f, 0.88f);        // Light gray
    static const FLinearColor FieldName(0.5f, 0.5f, 0.58f);      // Dim gray
    static const FLinearColor Dim(0.4f, 0.4f, 0.4f);             // Dark gray
    static const FLinearColor SectionHeader(0.95f, 0.85f, 0.4f); // Yellow
    static const FLinearColor Running(0.4f, 0.9f, 0.4f);         // Green
    static const FLinearColor Blocked(0.9f, 0.6f, 0.2f);         // Orange
    static const FLinearColor Yielded(0.85f, 0.85f, 0.4f);       // Yellow
    static const FLinearColor Failed(0.9f, 0.3f, 0.3f);          // Red
}

static FSlateColor GetStatusColor(const FString& Status)
{
    if (Status == TEXT("Running") || Status == TEXT("Ready"))  return FSlateColor(VMColors::Running);
    if (Status == TEXT("Blocked"))   return FSlateColor(VMColors::Blocked);
    if (Status == TEXT("Yielded"))   return FSlateColor(VMColors::Yielded);
    if (Status == TEXT("Failed"))    return FSlateColor(VMColors::Failed);
    return FSlateColor(VMColors::Value);
}

// ============================================================================
// Construct / Destruct
// ============================================================================

SHktVMStatePanel::~SHktVMStatePanel()
{
    FHktCoreDataCollector::Get().DisableCollection(TEXT("VMDetail"));
}

void SHktVMStatePanel::Construct(const FArguments& InArgs)
{
    FHktCoreDataCollector::Get().EnableCollection(TEXT("VMDetail"));

    ChildSlot
    [
        SNew(SVerticalBox)

        // ── 헤더 바: Source 선택 + 요약 ──
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
                .ColorAndOpacity(FSlateColor(VMColors::FieldName))
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
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SAssignNew(SummaryText, STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                .ColorAndOpacity(FSlateColor(VMColors::Header))
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
                RefreshData();
            })
        ]

        // ── 메인: 상단(Entity + VM 리스트) / 하단(상세) ──
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        [
            SNew(SSplitter)
            .Orientation(Orient_Vertical)

            + SSplitter::Slot()
            .Value(0.45f)
            [
                SNew(SSplitter)
                .Orientation(Orient_Horizontal)

                // 좌: Entity 리스트
                + SSplitter::Slot()
                .Value(0.4f)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(4, 2)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("EntityListHeader", "Entities"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                        .ColorAndOpacity(FSlateColor(VMColors::SectionHeader))
                    ]
                    + SVerticalBox::Slot()
                    .FillHeight(1.0f)
                    [
                        SAssignNew(EntityListView, SListView<TSharedPtr<FHktVMEntityRow>>)
                        .ListItemsSource(&EntityRows)
                        .OnGenerateRow(this, &SHktVMStatePanel::OnGenerateEntityRow)
                        .OnSelectionChanged_Lambda([this](TSharedPtr<FHktVMEntityRow> Item, ESelectInfo::Type SelectInfo)
                        {
                            if (SelectInfo == ESelectInfo::Direct) return;
                            SelectedEntityKey = Item.IsValid() ? Item->EntityKey : FString();
                            SelectedVMKey.Empty();
                            RebuildVMList();
                            BuildDetailPanel();
                        })
                        .SelectionMode(ESelectionMode::Single)
                    ]
                ]

                // 우: VM 리스트
                + SSplitter::Slot()
                .Value(0.6f)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(4, 2)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("VMListHeader", "Active VMs"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                        .ColorAndOpacity(FSlateColor(VMColors::SectionHeader))
                    ]
                    + SVerticalBox::Slot()
                    .FillHeight(1.0f)
                    [
                        SAssignNew(VMListView, SListView<TSharedPtr<FHktVMRow>>)
                        .ListItemsSource(&FilteredVMRows)
                        .OnGenerateRow(this, &SHktVMStatePanel::OnGenerateVMRow)
                        .OnSelectionChanged_Lambda([this](TSharedPtr<FHktVMRow> Item, ESelectInfo::Type SelectInfo)
                        {
                            if (SelectInfo == ESelectInfo::Direct) return;
                            SelectedVMKey = Item.IsValid() ? Item->VMKey : FString();
                            BuildDetailPanel();
                        })
                        .SelectionMode(ESelectionMode::Single)
                    ]
                ]
            ]

            // 하: VM 상세
            + SSplitter::Slot()
            .Value(0.55f)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    SAssignNew(DetailContainer, SVerticalBox)
                ]
            ]
        ]
    ];

    CachedVersion = FHktCoreDataCollector::Get().GetVersion();
    RebuildSourceOptions();
    RefreshData();

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

void SHktVMStatePanel::RebuildSourceOptions()
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

void SHktVMStatePanel::RefreshData()
{
    if (SelectedSource.IsEmpty())
    {
        RebuildSourceOptions();
        if (SelectedSource.IsEmpty()) return;
    }

    // ── 1. WorldState에서 전체 Entity 목록 읽기 ──
    const FString WsCategory = FString::Printf(TEXT("WorldState.%s"), *SelectedSource);
    TArray<TPair<FString, FString>> WsEntries = FHktCoreDataCollector::Get().GetEntries(WsCategory);

    // Entity 목록 파싱
    TMap<FString, TSharedPtr<FHktVMEntityRow>> NewEntityMap;
    for (const auto& Entry : WsEntries)
    {
        if (!Entry.Key.StartsWith(TEXT("E_"))) continue;

        TSharedPtr<FHktVMEntityRow> Row = MakeShared<FHktVMEntityRow>();
        Row->EntityKey = Entry.Key;
        Row->VMCount = 0;

        // DebugName 파싱
        TArray<FString> Segments;
        Entry.Value.ParseIntoArray(Segments, TEXT("|"), true);
        for (FString& Seg : Segments)
        {
            Seg.TrimStartAndEndInline();
            if (Seg.StartsWith(TEXT("DebugName=")))
            {
                Row->DebugName = Seg.Mid(10);
                break;
            }
        }

        NewEntityMap.Add(Entry.Key, Row);
    }

    // ── 2. VMDetail에서 VM 데이터 읽기 ──
    TArray<TPair<FString, FString>> VMEntries = FHktCoreDataCollector::Get().GetEntries(TEXT("VMDetail"));

    // Entity별 VM 요약 (VMDetail의 E_ 행)
    TMap<FString, TPair<int32, FString>> VMSummaryMap;  // EntityKey → (VMCount, VMNames)
    AllVMRows.Reset();

    for (const auto& Entry : VMEntries)
    {
        if (Entry.Key.StartsWith(TEXT("E_")))
        {
            // VMDetail Entity 요약: VMCount=N | Names=Tag1,Tag2
            int32 Count = 0;
            FString Names;
            TArray<FString> Segments;
            Entry.Value.ParseIntoArray(Segments, TEXT("|"), true);
            for (FString& Seg : Segments)
            {
                Seg.TrimStartAndEndInline();
                int32 EqIdx;
                if (Seg.FindChar(TEXT('='), EqIdx) && EqIdx > 0)
                {
                    FString Field = Seg.Left(EqIdx);
                    FString Val = Seg.Mid(EqIdx + 1);
                    if (Field == TEXT("VMCount"))  Count = FCString::Atoi(*Val);
                    else if (Field == TEXT("Names"))  Names = Val;
                }
            }
            VMSummaryMap.Add(Entry.Key, TPair<int32, FString>(Count, Names));
        }
        else if (Entry.Key.StartsWith(TEXT("VM_")))
        {
            // VM 상세 행
            TSharedPtr<FHktVMRow> Row = MakeShared<FHktVMRow>();
            Row->VMKey = Entry.Key;

            TArray<FString> Segments;
            Entry.Value.ParseIntoArray(Segments, TEXT("|"), true);
            for (FString& Seg : Segments)
            {
                Seg.TrimStartAndEndInline();
                int32 EqIdx;
                if (Seg.FindChar(TEXT('='), EqIdx) && EqIdx > 0)
                {
                    Row->Props.Add(Seg.Left(EqIdx), Seg.Mid(EqIdx + 1));
                }
            }

            if (const FString* Src = Row->Props.Find(TEXT("Src")))
            {
                Row->EntityKey = FString::Printf(TEXT("E_%s"), **Src);
            }

            AllVMRows.Add(Row);
        }
    }

    // ── 3. WorldState entity에 VM 요약 병합 ──
    for (auto& KV : VMSummaryMap)
    {
        if (TSharedPtr<FHktVMEntityRow>* EntityRow = NewEntityMap.Find(KV.Key))
        {
            (*EntityRow)->VMCount = KV.Value.Key;
            (*EntityRow)->VMNames = KV.Value.Value;
        }
    }

    // ── 4. Entity 리스트 구성 (필터 적용) ──
    EntityRows.Reset();
    for (auto& KV : NewEntityMap)
    {
        if (!FilterText.IsEmpty())
        {
            bool bMatch = KV.Value->EntityKey.Contains(FilterText)
                || KV.Value->DebugName.Contains(FilterText)
                || KV.Value->VMNames.Contains(FilterText);
            if (!bMatch) continue;
        }
        EntityRows.Add(KV.Value);
    }
    EntityRows.Sort([](const TSharedPtr<FHktVMEntityRow>& A, const TSharedPtr<FHktVMEntityRow>& B)
    {
        int32 IdA = FCString::Atoi(*A->EntityKey.Mid(2));
        int32 IdB = FCString::Atoi(*B->EntityKey.Mid(2));
        return IdA < IdB;
    });

    // 요약 텍스트
    SummaryText->SetText(FText::FromString(
        FString::Printf(TEXT("Active VMs: %d  |  Entities: %d"), AllVMRows.Num(), NewEntityMap.Num())));

    EntityListView->RequestListRefresh();

    // 선택 상태 유지
    if (!SelectedEntityKey.IsEmpty())
    {
        if (!NewEntityMap.Contains(SelectedEntityKey))
        {
            SelectedEntityKey.Empty();
            SelectedVMKey.Empty();
        }
        else
        {
            for (const auto& Row : EntityRows)
            {
                if (Row->EntityKey == SelectedEntityKey)
                {
                    EntityListView->SetSelection(Row, ESelectInfo::Direct);
                    break;
                }
            }
        }
    }

    RebuildVMList();

    // VM 선택 복원
    if (!SelectedVMKey.IsEmpty())
    {
        bool bFound = false;
        for (const auto& Row : FilteredVMRows)
        {
            if (Row->VMKey == SelectedVMKey)
            {
                VMListView->SetSelection(Row, ESelectInfo::Direct);
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            SelectedVMKey.Empty();
        }
    }

    BuildDetailPanel();
}

// ============================================================================
// VM List (선택된 Entity의 VM만 필터링)
// ============================================================================

void SHktVMStatePanel::RebuildVMList()
{
    FilteredVMRows.Reset();

    if (SelectedEntityKey.IsEmpty())
    {
        // Entity 미선택 시 전체 VM 표시
        FilteredVMRows = AllVMRows;
    }
    else
    {
        for (const auto& Row : AllVMRows)
        {
            if (Row->EntityKey == SelectedEntityKey)
            {
                FilteredVMRows.Add(Row);
            }
        }
    }

    VMListView->RequestListRefresh();
}

// ============================================================================
// Entity Row 렌더링
// ============================================================================

TSharedRef<ITableRow> SHktVMStatePanel::OnGenerateEntityRow(
    TSharedPtr<FHktVMEntityRow> Item,
    const TSharedRef<STableViewBase>& OwnerTable)
{
    // 표시: "E_1  [Goblin]  (2 VMs) Attack,Move"  또는  "E_3  [NPC]"
    FString Label = Item->EntityKey;
    if (!Item->DebugName.IsEmpty())
    {
        Label += FString::Printf(TEXT("  [%s]"), *Item->DebugName);
    }
    if (Item->VMCount > 0)
    {
        Label += FString::Printf(TEXT("  (%d VMs) %s"), Item->VMCount, *Item->VMNames);
    }

    FLinearColor Color = (Item->VMCount > 0) ? VMColors::EntityKey : VMColors::EntityDim;

    return SNew(STableRow<TSharedPtr<FHktVMEntityRow>>, OwnerTable)
    [
        SNew(STextBlock)
        .Text(FText::FromString(Label))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
        .ColorAndOpacity(FSlateColor(Color))
        .Margin(FMargin(4, 2))
    ];
}

// ============================================================================
// VM Row 렌더링
// ============================================================================

TSharedRef<ITableRow> SHktVMStatePanel::OnGenerateVMRow(
    TSharedPtr<FHktVMRow> Item,
    const TSharedRef<STableViewBase>& OwnerTable)
{
    const FString* Status = Item->Props.Find(TEXT("Status"));
    const FString* Event = Item->Props.Find(TEXT("Event"));
    const FString* PC = Item->Props.Find(TEXT("PC"));
    const FString* CodeSize = Item->Props.Find(TEXT("CodeSize"));

    FString StatusStr = Status ? *Status : TEXT("?");
    FString EventStr = Event ? *Event : TEXT("?");
    FString PCStr = PC ? *PC : TEXT("?");
    FString CodeSizeStr = CodeSize ? *CodeSize : TEXT("?");

    FString Label = FString::Printf(TEXT("%s: %s  [%s]  PC=%s/%s"),
        *Item->VMKey, *EventStr, *StatusStr, *PCStr, *CodeSizeStr);

    return SNew(STableRow<TSharedPtr<FHktVMRow>>, OwnerTable)
    [
        SNew(STextBlock)
        .Text(FText::FromString(Label))
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
        .ColorAndOpacity(GetStatusColor(StatusStr))
        .Margin(FMargin(4, 2))
    ];
}

// ============================================================================
// Detail Panel — 선택된 VM의 상세 정보
// ============================================================================

void SHktVMStatePanel::BuildDetailPanel()
{
    DetailContainer->ClearChildren();

    if (SelectedVMKey.IsEmpty())
    {
        FText Hint = SelectedEntityKey.IsEmpty()
            ? LOCTEXT("NoEntitySelection", "Select an entity to view its VMs")
            : LOCTEXT("NoVMSelection", "No active VMs for this entity");

        // 선택된 entity가 있고 VM이 FilteredVMRows에 있으면 VM 선택 안내
        if (!SelectedEntityKey.IsEmpty() && FilteredVMRows.Num() > 0)
        {
            Hint = LOCTEXT("SelectVM", "Select a VM to view details");
        }

        DetailContainer->AddSlot()
        .AutoHeight()
        .Padding(4.f)
        [
            SNew(STextBlock)
            .Text(Hint)
            .ColorAndOpacity(FSlateColor(VMColors::Dim))
            .Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
        ];
        return;
    }

    // 선택된 VM 찾기
    TSharedPtr<FHktVMRow> SelectedVM;
    for (const auto& Row : AllVMRows)
    {
        if (Row->VMKey == SelectedVMKey)
        {
            SelectedVM = Row;
            break;
        }
    }
    if (!SelectedVM.IsValid()) return;

    const TMap<FString, FString>& P = SelectedVM->Props;

    auto GetProp = [&P](const TCHAR* Key) -> FString
    {
        if (const FString* V = P.Find(Key)) return *V;
        return TEXT("-");
    };

    // ── 헤더: VM Key + Event Tag ──
    DetailContainer->AddSlot()
    .AutoHeight()
    .Padding(4, 4, 4, 2)
    [
        SNew(STextBlock)
        .Text(FText::FromString(FString::Printf(TEXT("%s  —  %s"),
            *SelectedVM->VMKey, *GetProp(TEXT("Event")))))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
        .ColorAndOpacity(FSlateColor(VMColors::VMKey))
    ];

    // ── 기본 정보 ──
    auto AddField = [this](const FString& Name, const FString& Value, FLinearColor ValColor = VMColors::Value)
    {
        DetailContainer->AddSlot()
        .AutoHeight()
        .Padding(8, 1)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SBox)
                .MinDesiredWidth(120.f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Name))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    .ColorAndOpacity(FSlateColor(VMColors::FieldName))
                ]
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Value))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                .ColorAndOpacity(FSlateColor(ValColor))
            ]
        ];
    };

    auto AddSectionHeader = [this](const FText& Title)
    {
        DetailContainer->AddSlot()
        .AutoHeight()
        .Padding(4, 6, 4, 2)
        [
            SNew(STextBlock)
            .Text(Title)
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
            .ColorAndOpacity(FSlateColor(VMColors::SectionHeader))
        ];
    };

    // Status with color
    FString StatusStr = GetProp(TEXT("Status"));
    FLinearColor StatusColor = VMColors::Value;
    if (StatusStr == TEXT("Running") || StatusStr == TEXT("Ready"))  StatusColor = VMColors::Running;
    else if (StatusStr == TEXT("Blocked"))  StatusColor = VMColors::Blocked;
    else if (StatusStr == TEXT("Yielded"))  StatusColor = VMColors::Yielded;
    else if (StatusStr == TEXT("Failed"))   StatusColor = VMColors::Failed;

    AddField(TEXT("Status"), StatusStr, StatusColor);
    AddField(TEXT("PC"), FString::Printf(TEXT("%s / %s"), *GetProp(TEXT("PC")), *GetProp(TEXT("CodeSize"))));
    AddField(TEXT("Last Op"), GetProp(TEXT("Op")));
    AddField(TEXT("Source Entity"), GetProp(TEXT("Src")));
    AddField(TEXT("Target Entity"), GetProp(TEXT("Tgt")));
    AddField(TEXT("Creation Frame"), GetProp(TEXT("CreationFrame")));
    AddField(TEXT("Player UID"), GetProp(TEXT("PlayerUid")));

    // ── Registers ──
    AddSectionHeader(LOCTEXT("RegistersHeader", "Registers"));

    // 범용 레지스터 (R0-R8) — 2열 그리드
    {
        TSharedRef<SHorizontalBox> RegRow1 = SNew(SHorizontalBox);
        TSharedRef<SHorizontalBox> RegRow2 = SNew(SHorizontalBox);

        const TCHAR* RegNames[] = { TEXT("R0"), TEXT("R1"), TEXT("R2"), TEXT("R3"), TEXT("R4"),
                                     TEXT("R5"), TEXT("R6"), TEXT("R7"), TEXT("R8") };
        for (int32 i = 0; i < 9; ++i)
        {
            TSharedRef<SHorizontalBox>& Row = (i < 5) ? RegRow1 : RegRow2;
            Row->AddSlot()
            .AutoWidth()
            .Padding(4, 0)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("%s="), RegNames[i])))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    .ColorAndOpacity(FSlateColor(VMColors::FieldName))
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SBox).MinDesiredWidth(50.f)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(GetProp(RegNames[i])))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                        .ColorAndOpacity(FSlateColor(VMColors::Value))
                    ]
                ]
            ];
        }

        DetailContainer->AddSlot().AutoHeight().Padding(4, 1) [ RegRow1 ];
        DetailContainer->AddSlot().AutoHeight().Padding(4, 1) [ RegRow2 ];
    }

    // 특수 레지스터
    {
        TSharedRef<SHorizontalBox> SpecRow1 = SNew(SHorizontalBox);
        TSharedRef<SHorizontalBox> SpecRow2 = SNew(SHorizontalBox);

        struct FSpecReg { const TCHAR* Display; const TCHAR* Key; };
        FSpecReg SpecRegs[] = {
            { TEXT("Self"), TEXT("Self") },
            { TEXT("Target"), TEXT("Target") },
            { TEXT("Spawned"), TEXT("Spawned") },
            { TEXT("Hit"), TEXT("Hit") },
            { TEXT("Iter"), TEXT("Iter") },
            { TEXT("Flag"), TEXT("Flag") },
        };

        for (int32 i = 0; i < 6; ++i)
        {
            TSharedRef<SHorizontalBox>& Row = (i < 3) ? SpecRow1 : SpecRow2;
            Row->AddSlot()
            .AutoWidth()
            .Padding(4, 0)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("%s="), SpecRegs[i].Display)))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    .ColorAndOpacity(FSlateColor(VMColors::FieldName))
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SBox).MinDesiredWidth(50.f)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(GetProp(SpecRegs[i].Key)))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                        .ColorAndOpacity(FSlateColor(VMColors::Value))
                    ]
                ]
            ];
        }

        DetailContainer->AddSlot().AutoHeight().Padding(4, 1) [ SpecRow1 ];
        DetailContainer->AddSlot().AutoHeight().Padding(4, 1) [ SpecRow2 ];
    }

    // ── Wait State ──
    AddSectionHeader(LOCTEXT("WaitStateHeader", "Wait State"));
    FString WaitType = GetProp(TEXT("WaitType"));
    FLinearColor WaitColor = (WaitType == TEXT("None")) ? VMColors::Dim : VMColors::Blocked;
    AddField(TEXT("Type"), WaitType, WaitColor);
    if (WaitType != TEXT("None") && WaitType != TEXT("-"))
    {
        AddField(TEXT("Watched Entity"), GetProp(TEXT("WaitEntity")));
        AddField(TEXT("Remaining Time"), GetProp(TEXT("WaitTime")));
        AddField(TEXT("Wait Frames"), GetProp(TEXT("WaitFrames")));
    }

    // ── Context Params ──
    AddSectionHeader(LOCTEXT("ContextHeader", "Context Params"));
    AddField(TEXT("Param0"), GetProp(TEXT("Param0")));
    AddField(TEXT("Param1"), GetProp(TEXT("Param1")));
    AddField(TEXT("TargetPos"), GetProp(TEXT("TargetPos")));
}

#undef LOCTEXT_NAMESPACE
