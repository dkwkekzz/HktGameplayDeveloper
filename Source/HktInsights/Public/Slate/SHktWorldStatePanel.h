// Copyright Hkt Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"

/**
 * FHktWorldStateEntityRow - 엔티티 행 데이터
 * EntityKey + 프로퍼티 맵 (컬럼명 → 값)
 */
struct FHktWorldStateEntityRow
{
    FString EntityKey;                   // "E_0", "E_1", ...
    TMap<FString, FString> Props;        // PropertyName → Value
};

/**
 * SHktWorldStatePanel
 *
 * "WorldState.*" 카테고리 데이터를 스프레드시트 형태로 표시.
 * - 가로축: 엔티티 (행), 세로축: 프로퍼티 (컬럼)
 * - 소스 선택 콤보박스 (Server/Client)
 * - 엔티티 필터링 및 선택 기능
 * - 선택된 엔티티 상세 정보 패널
 */
class HKTINSIGHTS_API SHktWorldStatePanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHktWorldStatePanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    void RefreshData();
    void RebuildSourceOptions();
    void RebuildGrid();
    void RebuildFilteredRows();
    void UpdateDetailPanel();

    TSharedRef<ITableRow> OnGenerateRow(
        TSharedPtr<FHktWorldStateEntityRow> Item,
        const TSharedRef<STableViewBase>& OwnerTable);

    // Source 선택
    TArray<TSharedPtr<FString>> SourceOptions;
    FString SelectedSource;

    // 메타 정보
    TSharedPtr<STextBlock> FrameText;
    TSharedPtr<STextBlock> EntityCountText;

    // 엔티티 그리드
    TArray<FString> CurrentColumns;    // 프로퍼티 컬럼 이름 (순서 보존)
    TSharedPtr<SHeaderRow> HeaderRow;
    TSharedPtr<SListView<TSharedPtr<FHktWorldStateEntityRow>>> ListView;
    TSharedPtr<SVerticalBox> GridContainer;

    TArray<TSharedPtr<FHktWorldStateEntityRow>> AllRows;
    TArray<TSharedPtr<FHktWorldStateEntityRow>> FilteredRows;
    FString FilterText;

    // 선택 & 상세
    TSharedPtr<FHktWorldStateEntityRow> SelectedEntity;
    TSharedPtr<SVerticalBox> DetailContainer;

    uint32 CachedVersion = 0;
};
