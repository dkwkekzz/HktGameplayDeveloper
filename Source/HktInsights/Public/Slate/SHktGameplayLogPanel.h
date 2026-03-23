// Copyright Hkt Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "GameplayTagContainer.h"

struct FHktLogEntry;

/**
 * SHktGameplayLogPanel
 *
 * HktGameplay 이벤트 로그를 시간순으로 표시하는 디버그 패널.
 * FHktCoreEventLog 싱글톤을 통해 링 버퍼에서 이벤트를 읽어 표시.
 *
 * - 패널 열림/닫힘에 따라 로그 수집 자동 활성화/비활성화
 * - Category(FGameplayTag) 계층적 필터링: MatchesTag()로 상위 태그 선택 시 하위 포함
 * - EntityId, EventTag, 텍스트 필터 지원
 * - 자동 스크롤 (하단 고정)
 */
class HKTINSIGHTS_API SHktGameplayLogPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHktGameplayLogPanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SHktGameplayLogPanel() override;

private:
    /** 새 로그 엔트리 폴링 및 필터 적용 */
    void PollNewEntries();

    /** 필터 조건에 매칭되는지 확인 */
    bool PassesFilter(const FHktLogEntry& Entry) const;

    /** 필터링된 행 목록 재구성 */
    void RebuildFilteredRows();

    /** 카테고리 체크박스 목록 재구성 */
    void RebuildCategoryCheckboxes();

    /** SListView 행 생성 콜백 */
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FHktLogEntry> Item,
                                         const TSharedRef<STableViewBase>& OwnerTable);

    /** 카테고리별 색상 */
    static FSlateColor GetCategoryColor(const FGameplayTag& Category);

    /** FGameplayTag에서 "HktLog." 접두사 제거 후 표시용 문자열 반환 */
    static FString GetCategoryDisplayName(const FGameplayTag& Category);

    // ── 데이터 ──

    /** 필터를 통과한 전체 로그 (표시용) */
    TArray<TSharedPtr<FHktLogEntry>> FilteredRows;

    /** 필터를 통과하지 않은 것 포함 전체 로그 (필터 변경 시 재적용용) */
    TArray<TSharedPtr<FHktLogEntry>> AllRows;

    /** 리스트뷰 */
    TSharedPtr<SListView<TSharedPtr<FHktLogEntry>>> ListView;

    /** 카테고리 체크박스 컨테이너 */
    TSharedPtr<SVerticalBox> CategoryContainer;

    /** 로그 카운트 텍스트 */
    TSharedPtr<STextBlock> LogCountText;

    // ── 필터 상태 ──
    FString FilterText;
    FString EntityIdFilterText;             // 쉼표 구분 엔티티 ID
    TSet<int32> EntityIdFilter;             // 파싱된 엔티티 ID 셋
    FGameplayTagContainer EnabledCategories;  // 활성화된 카테고리 태그
    FGameplayTagContainer KnownCategories;    // 지금까지 본 카테고리 태그

    // ── 폴링 상태 ──
    uint32 ReadIndex = 0;
    uint32 CachedVersion = 0;
    bool bAutoScroll = true;
    double StartTime = 0.0;           // 패널 시작 시간 (상대 시간 표시용)
};
