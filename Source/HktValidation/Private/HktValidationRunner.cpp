// Copyright Hkt Studios, Inc. All Rights Reserved.

#include "HktValidationLog.h"
#include "HktValidationTypes.h"
#include "HktValidationTestHarness.h"
#include "HktStoryBuilder.h"
#include "HktStoryValidator.h"
#include "HktStoryRegistry.h"
#include "VM/HktVMProgram.h"

// 외부 테스트 함수 선언
namespace HktOpcodeTests
{
	FHktTestReport RunControlFlowTests();
	FHktTestReport RunDataTests();
	FHktTestReport RunEntityTests();
	FHktTestReport RunCompositeTests();
}

namespace HktStoryIntegrityTests
{
	FHktTestReport RunAllIntegrityTests();
}

namespace HktValidationRunner
{
	static FHktTestReport LastReport;

	static void PrintReport(const FHktTestReport& Report)
	{
		UE_LOG(LogHktValidation, Log, TEXT(""));
		UE_LOG(LogHktValidation, Log, TEXT("============================================"));
		UE_LOG(LogHktValidation, Log, TEXT("  HktValidation Report: %d/%d PASSED"),
			Report.PassCount(), Report.TotalCount());
		UE_LOG(LogHktValidation, Log, TEXT("============================================"));

		for (const FHktTestResult& R : Report.Results)
		{
			if (R.bPassed)
			{
				UE_LOG(LogHktValidation, Log, TEXT("  [PASS] %s"), *R.TestName);
			}
			else
			{
				UE_LOG(LogHktValidation, Error, TEXT("  [FAIL] %s — %s"), *R.TestName, *R.Message);
			}
		}

		UE_LOG(LogHktValidation, Log, TEXT("============================================"));
		if (Report.AllPassed())
		{
			UE_LOG(LogHktValidation, Log, TEXT("  ALL TESTS PASSED"));
		}
		else
		{
			UE_LOG(LogHktValidation, Error, TEXT("  %d TESTS FAILED"), Report.FailCount());
		}
		UE_LOG(LogHktValidation, Log, TEXT("============================================"));
	}

	void RunOpcodeTests()
	{
		UE_LOG(LogHktValidation, Log, TEXT("[HktValidation] Running Opcode tests..."));

		FHktTestReport Report;
		Report.Append(HktOpcodeTests::RunControlFlowTests());
		Report.Append(HktOpcodeTests::RunDataTests());
		Report.Append(HktOpcodeTests::RunEntityTests());
		Report.Append(HktOpcodeTests::RunCompositeTests());

		LastReport = Report;
		PrintReport(Report);
	}

	void RunStoryTests()
	{
		UE_LOG(LogHktValidation, Log, TEXT("[HktValidation] Running Story integrity tests..."));

		FHktTestReport Report = HktStoryIntegrityTests::RunAllIntegrityTests();

		LastReport = Report;
		PrintReport(Report);
	}

	void RunAllTests()
	{
		UE_LOG(LogHktValidation, Log, TEXT("[HktValidation] Running ALL tests..."));

		FHktTestReport Report;
		Report.Append(HktOpcodeTests::RunControlFlowTests());
		Report.Append(HktOpcodeTests::RunDataTests());
		Report.Append(HktOpcodeTests::RunEntityTests());
		Report.Append(HktOpcodeTests::RunCompositeTests());
		Report.Append(HktStoryIntegrityTests::RunAllIntegrityTests());

		LastReport = Report;
		PrintReport(Report);
	}

	void PrintLastReport()
	{
		if (LastReport.TotalCount() == 0)
		{
			UE_LOG(LogHktValidation, Warning, TEXT("No test results. Run hkt.validation.run first."));
			return;
		}
		PrintReport(LastReport);
	}
}
