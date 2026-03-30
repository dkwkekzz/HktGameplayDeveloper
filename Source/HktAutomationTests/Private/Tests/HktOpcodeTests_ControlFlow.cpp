// Copyright Hkt Studios, Inc. All Rights Reserved.

#include "HktAutomationTestsLog.h"
#include "HktAutomationTestsTypes.h"
#include "HktAutomationTestsHarness.h"
#include "HktStoryBuilder.h"
#include "VM/HktVMProgram.h"

namespace HktOpcodeTests
{

// ============================================================================
// 헬퍼: 테스트용 GameplayTag 생성
// ============================================================================

static FGameplayTag TestTag(const TCHAR* Name)
{
	return FGameplayTag::RequestGameplayTag(FName(Name), false);
}

static FGameplayTag TestStoryTag()
{
	static FGameplayTag Tag = TestTag(TEXT("Test.Validation.Opcode"));
	return Tag;
}

// ============================================================================
// Control Flow Tests
// ============================================================================

static FHktTestResult Test_Nop()
{
	FHktAutomationTestHarness H;
	H.Setup();
	FHktEntityId E = H.CreateEntity();

	auto Program = FHktStoryBuilder::Create(TestStoryTag())
		.LoadConst(Reg::R0, 42)
		.Halt()
		.Build();

	EVMStatus Status = H.ExecuteProgram(Program, E);
	H.Teardown();

	if (Status != EVMStatus::Completed)
		return FHktTestResult::Fail(TEXT("Nop_Halt"), TEXT("Expected Completed"));
	if (H.GetRegister(Reg::R0) != 42)
		return FHktTestResult::Fail(TEXT("Nop_Halt"), TEXT("R0 should be 42"));

	return FHktTestResult::Pass(TEXT("Nop_Halt"));
}

static FHktTestResult Test_Halt()
{
	FHktAutomationTestHarness H;
	H.Setup();
	FHktEntityId E = H.CreateEntity();

	auto Program = FHktStoryBuilder::Create(TestStoryTag())
		.Halt()
		.Build();

	EVMStatus Status = H.ExecuteProgram(Program, E);
	H.Teardown();

	if (Status != EVMStatus::Completed)
		return FHktTestResult::Fail(TEXT("Halt"), TEXT("Expected Completed"));

	return FHktTestResult::Pass(TEXT("Halt"));
}

static FHktTestResult Test_Fail()
{
	FHktAutomationTestHarness H;
	H.Setup();
	FHktEntityId E = H.CreateEntity();

	auto Program = FHktStoryBuilder::Create(TestStoryTag())
		.Fail()
		.Build();

	EVMStatus Status = H.ExecuteProgram(Program, E);
	H.Teardown();

	if (Status != EVMStatus::Failed)
		return FHktTestResult::Fail(TEXT("Fail"), TEXT("Expected Failed"));

	return FHktTestResult::Pass(TEXT("Fail"));
}

static FHktTestResult Test_Yield()
{
	FHktAutomationTestHarness H;
	H.Setup();
	FHktEntityId E = H.CreateEntity();

	auto Program = FHktStoryBuilder::Create(TestStoryTag())
		.LoadConst(Reg::R0, 10)
		.Yield(1)
		.LoadConst(Reg::R0, 20)
		.Halt()
		.Build();

	// 첫 번째 실행: Yield에서 정지
	EVMStatus Status = H.ExecuteProgram(Program, E);
	H.Teardown();

	// ExecuteProgram은 Yield를 자동 소비하므로 Completed가 되어야 함
	if (Status != EVMStatus::Completed)
		return FHktTestResult::Fail(TEXT("Yield"), TEXT("Expected Completed after auto-resume"));
	if (H.GetRegister(Reg::R0) != 20)
		return FHktTestResult::Fail(TEXT("Yield"), TEXT("R0 should be 20 after resume"));

	return FHktTestResult::Pass(TEXT("Yield"));
}

static FHktTestResult Test_YieldSeconds()
{
	FHktAutomationTestHarness H;
	H.Setup();
	FHktEntityId E = H.CreateEntity();

	auto Program = FHktStoryBuilder::Create(TestStoryTag())
		.LoadConst(Reg::R0, 1)
		.WaitSeconds(0.5f)
		.LoadConst(Reg::R0, 2)
		.Halt()
		.Build();

	// ExecuteProgram은 Timer를 자동 진행하므로 Completed가 되어야 함
	EVMStatus Status = H.ExecuteProgram(Program, E);
	H.Teardown();

	if (Status != EVMStatus::Completed)
		return FHktTestResult::Fail(TEXT("YieldSeconds"), TEXT("Expected Completed"));
	if (H.GetRegister(Reg::R0) != 2)
		return FHktTestResult::Fail(TEXT("YieldSeconds"), TEXT("R0 should be 2 after timer"));

	return FHktTestResult::Pass(TEXT("YieldSeconds"));
}

static FHktTestResult Test_Jump()
{
	FHktAutomationTestHarness H;
	H.Setup();
	FHktEntityId E = H.CreateEntity();

	auto Program = FHktStoryBuilder::Create(TestStoryTag())
		.LoadConst(Reg::R0, 1)
		.Jump(TEXT("skip"))
		.LoadConst(Reg::R0, 999)  // 이 줄은 스킵되어야 함
		.Label(TEXT("skip"))
		.LoadConst(Reg::R1, 2)
		.Halt()
		.Build();

	EVMStatus Status = H.ExecuteProgram(Program, E);
	H.Teardown();

	if (Status != EVMStatus::Completed)
		return FHktTestResult::Fail(TEXT("Jump"), TEXT("Expected Completed"));
	if (H.GetRegister(Reg::R0) != 1)
		return FHktTestResult::Fail(TEXT("Jump"), TEXT("R0 should be 1 (skip should have bypassed 999)"));
	if (H.GetRegister(Reg::R1) != 2)
		return FHktTestResult::Fail(TEXT("Jump"), TEXT("R1 should be 2"));

	return FHktTestResult::Pass(TEXT("Jump"));
}

static FHktTestResult Test_JumpIf_JumpIfNot()
{
	FHktAutomationTestHarness H;
	H.Setup();
	FHktEntityId E = H.CreateEntity();

	// JumpIf: R0=1 → 점프
	{
		auto Program = FHktStoryBuilder::Create(TestStoryTag())
			.LoadConst(Reg::R0, 1)
			.JumpIf(Reg::R0, TEXT("taken"))
			.LoadConst(Reg::R1, 0)
			.Halt()
			.Label(TEXT("taken"))
			.LoadConst(Reg::R1, 100)
			.Halt()
			.Build();

		EVMStatus Status = H.ExecuteProgram(Program, E);
		if (H.GetRegister(Reg::R1) != 100)
			return FHktTestResult::Fail(TEXT("JumpIf_JumpIfNot"), TEXT("JumpIf should jump when cond != 0"));
	}

	// JumpIf: R0=0 → 점프 안 함
	H.Setup();
	E = H.CreateEntity();
	{
		auto Program = FHktStoryBuilder::Create(TestStoryTag())
			.LoadConst(Reg::R0, 0)
			.JumpIf(Reg::R0, TEXT("taken"))
			.LoadConst(Reg::R1, 50)
			.Halt()
			.Label(TEXT("taken"))
			.LoadConst(Reg::R1, 100)
			.Halt()
			.Build();

		H.ExecuteProgram(Program, E);
		if (H.GetRegister(Reg::R1) != 50)
			return FHktTestResult::Fail(TEXT("JumpIf_JumpIfNot"), TEXT("JumpIf should not jump when cond == 0"));
	}

	// JumpIfNot: R0=0 → 점프
	H.Setup();
	E = H.CreateEntity();
	{
		auto Program = FHktStoryBuilder::Create(TestStoryTag())
			.LoadConst(Reg::R0, 0)
			.JumpIfNot(Reg::R0, TEXT("taken"))
			.LoadConst(Reg::R1, 50)
			.Halt()
			.Label(TEXT("taken"))
			.LoadConst(Reg::R1, 200)
			.Halt()
			.Build();

		H.ExecuteProgram(Program, E);
		if (H.GetRegister(Reg::R1) != 200)
			return FHktTestResult::Fail(TEXT("JumpIf_JumpIfNot"), TEXT("JumpIfNot should jump when cond == 0"));
	}

	H.Teardown();
	return FHktTestResult::Pass(TEXT("JumpIf_JumpIfNot"));
}

// ============================================================================
// Public: 테스트 실행
// ============================================================================

FHktTestReport RunControlFlowTests()
{
	FHktTestReport Report;
	Report.Add(Test_Nop());
	Report.Add(Test_Halt());
	Report.Add(Test_Fail());
	Report.Add(Test_Yield());
	Report.Add(Test_YieldSeconds());
	Report.Add(Test_Jump());
	Report.Add(Test_JumpIf_JumpIfNot());
	return Report;
}

} // namespace HktOpcodeTests
