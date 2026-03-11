// Copyright Hkt Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SHktInsightTable;

/**
 * SHktVMStatePanel
 *
 * "VM" 카테고리 데이터를 테이블로 표시.
 * VM 생성/틱/완료 및 Intent 상태를 Key-Value 형태로 보여줌.
 */
class HKTINSIGHTS_API SHktVMStatePanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHktVMStatePanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    void RefreshData();

    TSharedPtr<SHktInsightTable> Table;
    FDelegateHandle DataChangedHandle;
};
