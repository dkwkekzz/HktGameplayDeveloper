// Copyright Hkt Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class SHktInsightTable;

/** Entity 행: 해당 entity에서 실행 중인 VM 요약 */
struct FHktVMEntityRow
{
    FString EntityKey;     // "E_1"
    int32 VMCount = 0;
    FString VMNames;       // "Attack,Move"
};

/** VM 행: 개별 VM의 모든 프로퍼티 */
struct FHktVMRow
{
    FString VMKey;         // "VM_0"
    FString EntityKey;     // "E_1" (Src 필드에서 파싱)
    TMap<FString, FString> Props;
};

/**
 * SHktVMStatePanel
 *
 * Entity별 활성 VM 목록과 선택된 VM의 상세 정보를 표시하는 3-pane 뷰어.
 * - 좌: Entity 리스트 (활성 VM이 있는 entity)
 * - 우: 선택된 entity의 VM 목록
 * - 하: 선택된 VM의 상세 정보 (레지스터, 대기 상태, 컨텍스트 파라미터 등)
 *
 * FHktCoreDataCollector "VMDetail" 카테고리를 폴링하여 실시간 갱신.
 */
class HKTINSIGHTS_API SHktVMStatePanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHktVMStatePanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    void RefreshData();
    void RebuildVMList();
    void BuildDetailPanel();

    // Entity 리스트 콜백
    TSharedRef<ITableRow> OnGenerateEntityRow(TSharedPtr<FHktVMEntityRow> Item, const TSharedRef<STableViewBase>& OwnerTable);
    // VM 리스트 콜백
    TSharedRef<ITableRow> OnGenerateVMRow(TSharedPtr<FHktVMRow> Item, const TSharedRef<STableViewBase>& OwnerTable);

    // Entity 데이터
    TArray<TSharedPtr<FHktVMEntityRow>> EntityRows;
    TSharedPtr<SListView<TSharedPtr<FHktVMEntityRow>>> EntityListView;
    FString SelectedEntityKey;

    // VM 데이터
    TArray<TSharedPtr<FHktVMRow>> AllVMRows;        // 전체 VM
    TArray<TSharedPtr<FHktVMRow>> FilteredVMRows;   // 선택된 entity의 VM
    TSharedPtr<SListView<TSharedPtr<FHktVMRow>>> VMListView;
    FString SelectedVMKey;

    // 상세 패널
    TSharedPtr<SVerticalBox> DetailContainer;

    // 요약 텍스트
    TSharedPtr<STextBlock> SummaryText;

    // 폴링
    uint32 CachedVersion = 0;
};
