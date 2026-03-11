// Copyright Hkt Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SHktInsightTable;

/**
 * SHktWorldStatePanel
 *
 * "WorldState.*" 카테고리 데이터를 테이블로 표시.
 * 소스(Server/Client) 선택 콤보박스 + SHktInsightTable.
 * UHktInsightsWorldSubsystem::OnDataChanged에 바인딩하여 변경 시에만 갱신.
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

    TSharedPtr<SHktInsightTable> Table;

    /** 사용 가능한 WorldState 소스 (Server, Client 등) */
    TArray<TSharedPtr<FString>> SourceOptions;
    FString SelectedSource;

    FDelegateHandle DataChangedHandle;
};
