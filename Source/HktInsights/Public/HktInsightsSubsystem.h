// Copyright Hkt Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HktInsightsSubsystem.generated.h"

/**
 * UHktInsightsWorldSubsystem
 *
 * FHktCoreDataCollector에 저장된 데이터의 수명 관리 및 변경 알림 담당.
 * - 월드 종료 시 FHktCoreDataCollector 클리어
 * - 타이머(0.1초)로 Version 변경 감지 → OnDataChanged 브로드캐스트
 * - Slate 패널은 이 델리게이트에 바인딩하여 데이터 변경 시에만 갱신
 */
UCLASS()
class HKTINSIGHTS_API UHktInsightsWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** 데이터 변경 시 브로드캐스트 (패널 갱신용) */
    DECLARE_MULTICAST_DELEGATE(FOnInsightsDataChanged);
    FOnInsightsDataChanged OnDataChanged;

    /** 현재 데이터 버전 */
    uint32 GetLastSeenVersion() const { return LastSeenVersion; }

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

private:
    /** 변경 감지 폴링 */
    void CheckForUpdates();

    FTimerHandle PollTimerHandle;
    uint32 LastSeenVersion = 0;
};
