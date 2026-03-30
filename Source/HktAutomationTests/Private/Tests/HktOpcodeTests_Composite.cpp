// Copyright Hkt Studios, Inc. All Rights Reserved.

#include "HktAutomationTestsLog.h"
#include "HktAutomationTestsTypes.h"
#include "HktAutomationTestsHarness.h"
#include "HktStoryBuilder.h"
#include "HktCoreProperties.h"
#include "VM/HktVMProgram.h"
#include "VM/HktVMInterpreter.h"

namespace HktOpcodeTests
{

static FGameplayTag CompositeTestTag()
{
	return FGameplayTag::RequestGameplayTag(FName(TEXT("Test.Validation.Composite")), false);
}

// ============================================================================
// Builder Composite Operations Tests
// ============================================================================

static FHktTestResult Test_GetPosition_SetPosition()
{
	FHktAutomationTestHarness H;
	H.Setup();

	TMap<uint16, int32> SelfProps;
	SelfProps.Add(PropertyId::PosX, 100);
	SelfProps.Add(PropertyId::PosY, 200);
	SelfProps.Add(PropertyId::PosZ, 300);
	FHktEntityId Self = H.CreateEntityWithProperties(SelfProps);
	FHktEntityId Target = H.CreateEntity();

	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.GetPosition(Reg::R0, Reg::Self)       // R0=100, R1=200, R2=300
		.SetPosition(Reg::Target, Reg::R0)      // Target.Pos = (100, 200, 300)
		.Halt()
		.Build();

	H.ExecuteProgram(Program, Self, Target);
	H.Teardown();

	if (H.GetRegister(Reg::R0) != 100 || H.GetRegister(Reg::R1) != 200 || H.GetRegister(Reg::R2) != 300)
		return FHktTestResult::Fail(TEXT("GetPosition_SetPosition"), TEXT("GetPosition failed"));

	if (H.GetProperty(Target, PropertyId::PosX) != 100 ||
		H.GetProperty(Target, PropertyId::PosY) != 200 ||
		H.GetProperty(Target, PropertyId::PosZ) != 300)
		return FHktTestResult::Fail(TEXT("GetPosition_SetPosition"), TEXT("SetPosition failed"));

	return FHktTestResult::Pass(TEXT("GetPosition_SetPosition"));
}

static FHktTestResult Test_MoveToward()
{
	FHktAutomationTestHarness H;
	H.Setup();

	FHktEntityId Self = H.CreateEntity();
	TMap<uint16, int32> TargetProps;
	FHktEntityId Target = H.CreateEntity();

	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.LoadConst(Reg::R0, 500)   // TargetX
		.LoadConst(Reg::R1, 600)   // TargetY
		.LoadConst(Reg::R2, 0)     // TargetZ
		.MoveToward(Reg::Self, Reg::R0, 150)
		.Halt()
		.Build();

	H.ExecuteProgram(Program, Self, Target);
	H.Teardown();

	if (H.GetProperty(Self, PropertyId::MoveTargetX) != 500)
		return FHktTestResult::Fail(TEXT("MoveToward"), TEXT("MoveTargetX should be 500"));
	if (H.GetProperty(Self, PropertyId::MoveTargetY) != 600)
		return FHktTestResult::Fail(TEXT("MoveToward"), TEXT("MoveTargetY should be 600"));
	if (H.GetProperty(Self, PropertyId::MoveForce) != 150)
		return FHktTestResult::Fail(TEXT("MoveToward"), TEXT("MoveForce should be 150"));
	if (H.GetProperty(Self, PropertyId::IsMoving) != 1)
		return FHktTestResult::Fail(TEXT("MoveToward"), TEXT("IsMoving should be 1"));

	return FHktTestResult::Pass(TEXT("MoveToward"));
}

static FHktTestResult Test_MoveForward()
{
	FHktAutomationTestHarness H;
	H.Setup();

	TMap<uint16, int32> SelfProps;
	SelfProps.Add(PropertyId::RotYaw, 90);
	FHktEntityId Self = H.CreateEntityWithProperties(SelfProps);

	FGameplayTag SpawnTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Entity.Test.MoveForward")), false);

	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.SpawnEntity(SpawnTag)
		.MoveForward(Reg::Spawned, 500)
		.Halt()
		.Build();

	H.ExecuteProgram(Program, Self);
	H.Teardown();

	FHktEntityId Spawned = static_cast<FHktEntityId>(H.GetRegister(Reg::Spawned));
	if (H.GetProperty(Spawned, PropertyId::MoveForce) != 500)
		return FHktTestResult::Fail(TEXT("MoveForward"), TEXT("MoveForce should be 500"));
	if (H.GetProperty(Spawned, PropertyId::IsMoving) != 1)
		return FHktTestResult::Fail(TEXT("MoveForward"), TEXT("IsMoving should be 1"));
	if (H.GetProperty(Spawned, PropertyId::RotYaw) != 90)
		return FHktTestResult::Fail(TEXT("MoveForward"), TEXT("RotYaw should be copied from Self (90)"));

	return FHktTestResult::Pass(TEXT("MoveForward"));
}

static FHktTestResult Test_StopMovement()
{
	FHktAutomationTestHarness H;
	H.Setup();

	TMap<uint16, int32> SelfProps;
	SelfProps.Add(PropertyId::IsMoving, 1);
	SelfProps.Add(PropertyId::MoveForce, 100);
	FHktEntityId Self = H.CreateEntityWithProperties(SelfProps);

	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.StopMovement(Reg::Self)
		.Halt()
		.Build();

	H.ExecuteProgram(Program, Self);
	H.Teardown();

	if (H.GetProperty(Self, PropertyId::IsMoving) != 0)
		return FHktTestResult::Fail(TEXT("StopMovement"), TEXT("IsMoving should be 0"));

	return FHktTestResult::Pass(TEXT("StopMovement"));
}

static FHktTestResult Test_ApplyDamage()
{
	FHktAutomationTestHarness H;
	H.Setup();

	FHktEntityId Self = H.CreateEntity();

	TMap<uint16, int32> TargetProps;
	TargetProps.Add(PropertyId::Health, 100);
	TargetProps.Add(PropertyId::Defense, 5);
	FHktEntityId Target = H.CreateEntityWithProperties(TargetProps);

	// ApplyDamageConst(Target, 30) → damage = max(1, 30-5) = 25 → Health = 75
	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.ApplyDamageConst(Reg::Target, 30)
		.Halt()
		.Build();

	H.ExecuteProgram(Program, Self, Target);
	H.Teardown();

	int32 Health = H.GetProperty(Target, PropertyId::Health);
	if (Health != 75)
		return FHktTestResult::Fail(TEXT("ApplyDamage"), FString::Printf(TEXT("Health should be 75, got %d"), Health));

	return FHktTestResult::Pass(TEXT("ApplyDamage"));
}

static FHktTestResult Test_ApplyDamage_ClampZero()
{
	FHktAutomationTestHarness H;
	H.Setup();

	FHktEntityId Self = H.CreateEntity();

	TMap<uint16, int32> TargetProps;
	TargetProps.Add(PropertyId::Health, 10);
	TargetProps.Add(PropertyId::Defense, 0);
	FHktEntityId Target = H.CreateEntityWithProperties(TargetProps);

	// ApplyDamageConst(Target, 999) → Health clamped to 0
	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.ApplyDamageConst(Reg::Target, 999)
		.Halt()
		.Build();

	H.ExecuteProgram(Program, Self, Target);
	H.Teardown();

	int32 Health = H.GetProperty(Target, PropertyId::Health);
	if (Health != 0)
		return FHktTestResult::Fail(TEXT("ApplyDamage_ClampZero"), FString::Printf(TEXT("Health should be 0, got %d"), Health));

	return FHktTestResult::Pass(TEXT("ApplyDamage_ClampZero"));
}

static FHktTestResult Test_ApplyDamage_MinDamage()
{
	FHktAutomationTestHarness H;
	H.Setup();

	FHktEntityId Self = H.CreateEntity();

	TMap<uint16, int32> TargetProps;
	TargetProps.Add(PropertyId::Health, 100);
	TargetProps.Add(PropertyId::Defense, 999);  // 방어력이 데미지보다 높음
	FHktEntityId Target = H.CreateEntityWithProperties(TargetProps);

	// ApplyDamageConst(Target, 5) → damage = max(1, 5-999) = 1 → Health = 99
	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.ApplyDamageConst(Reg::Target, 5)
		.Halt()
		.Build();

	H.ExecuteProgram(Program, Self, Target);
	H.Teardown();

	int32 Health = H.GetProperty(Target, PropertyId::Health);
	if (Health != 99)
		return FHktTestResult::Fail(TEXT("ApplyDamage_MinDamage"), FString::Printf(TEXT("Health should be 99, got %d"), Health));

	return FHktTestResult::Pass(TEXT("ApplyDamage_MinDamage"));
}

static FHktTestResult Test_ForEachInRadius()
{
	FHktAutomationTestHarness H;
	H.Setup();

	TMap<uint16, int32> CenterProps;
	CenterProps.Add(PropertyId::PosX, 0);
	CenterProps.Add(PropertyId::PosY, 0);
	CenterProps.Add(PropertyId::PosZ, 0);
	CenterProps.Add(PropertyId::CollisionMask, 0);
	CenterProps.Add(PropertyId::Health, 100);
	FHktEntityId Center = H.CreateEntityWithProperties(CenterProps);

	// 범위 내 엔티티 2개
	TMap<uint16, int32> NearProps;
	NearProps.Add(PropertyId::PosX, 50);
	NearProps.Add(PropertyId::PosY, 0);
	NearProps.Add(PropertyId::PosZ, 0);
	NearProps.Add(PropertyId::Health, 100);
	H.CreateEntityWithProperties(NearProps);

	NearProps[PropertyId::PosX] = -50;
	H.CreateEntityWithProperties(NearProps);

	// R0에 루프 카운트 저장
	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.LoadConst(Reg::R0, 0)
		.ForEachInRadius(Reg::Self, 200)
			.AddImm(Reg::R0, Reg::R0, 1)
		.EndForEach()
		.Halt()
		.Build();

	H.ExecuteProgram(Program, Center);
	H.Teardown();

	if (H.GetRegister(Reg::R0) != 2)
		return FHktTestResult::Fail(TEXT("ForEachInRadius"), FString::Printf(TEXT("Loop count should be 2, got %d"), H.GetRegister(Reg::R0)));

	return FHktTestResult::Pass(TEXT("ForEachInRadius"));
}

static FHktTestResult Test_ForEachInRadius_Empty()
{
	FHktAutomationTestHarness H;
	H.Setup();

	TMap<uint16, int32> CenterProps;
	CenterProps.Add(PropertyId::PosX, 0);
	CenterProps.Add(PropertyId::PosY, 0);
	CenterProps.Add(PropertyId::PosZ, 0);
	CenterProps.Add(PropertyId::CollisionMask, 0);
	FHktEntityId Center = H.CreateEntityWithProperties(CenterProps);

	// 범위 내 엔티티 없음 → 루프 0회
	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.LoadConst(Reg::R0, 0)
		.ForEachInRadius(Reg::Self, 10)  // 10cm → 아무것도 없음
			.AddImm(Reg::R0, Reg::R0, 1)
		.EndForEach()
		.Halt()
		.Build();

	H.ExecuteProgram(Program, Center);
	H.Teardown();

	if (H.GetRegister(Reg::R0) != 0)
		return FHktTestResult::Fail(TEXT("ForEachInRadius_Empty"), TEXT("Loop count should be 0"));

	return FHktTestResult::Pass(TEXT("ForEachInRadius_Empty"));
}

// ============================================================================
// Precondition Tests
// ============================================================================

static FHktTestResult Test_Precondition_Pass()
{
	FHktAutomationTestHarness H;
	H.Setup();
	FHktEntityId Self = H.CreateEntity();
	FHktEntityId Target = H.CreateEntity();

	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.BeginPrecondition()
			.LoadConst(Reg::Flag, 1)  // Flag != 0 → pass
			.Halt()
		.EndPrecondition()
		.LoadConst(Reg::R0, 42)
		.Halt()
		.Build();

	if (!Program)
		return FHktTestResult::Fail(TEXT("Precondition_Pass"), TEXT("Program build failed"));

	// Precondition 검증
	FHktEvent TestEvent;
	TestEvent.SourceEntity = Self;
	TestEvent.TargetEntity = Target;

	bool bPass = FHktVMInterpreter::ExecutePrecondition(
		Program->PreconditionCode,
		Program->PreconditionConstants,
		Program->PreconditionStrings,
		H.GetWorldState(),
		TestEvent);

	H.Teardown();

	if (!bPass)
		return FHktTestResult::Fail(TEXT("Precondition_Pass"), TEXT("Precondition should pass when Flag=1"));

	return FHktTestResult::Pass(TEXT("Precondition_Pass"));
}

static FHktTestResult Test_Precondition_Fail()
{
	FHktAutomationTestHarness H;
	H.Setup();
	FHktEntityId Self = H.CreateEntity();

	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.BeginPrecondition()
			.Fail()  // Fail opcode → precondition fails
		.EndPrecondition()
		.Halt()
		.Build();

	if (!Program)
		return FHktTestResult::Fail(TEXT("Precondition_Fail"), TEXT("Program build failed"));

	FHktEvent TestEvent;
	TestEvent.SourceEntity = Self;

	bool bPass = FHktVMInterpreter::ExecutePrecondition(
		Program->PreconditionCode,
		Program->PreconditionConstants,
		Program->PreconditionStrings,
		H.GetWorldState(),
		TestEvent);

	H.Teardown();

	if (bPass)
		return FHktTestResult::Fail(TEXT("Precondition_Fail"), TEXT("Precondition should fail on Fail opcode"));

	return FHktTestResult::Pass(TEXT("Precondition_Fail"));
}

static FHktTestResult Test_Precondition_ReadOnly()
{
	FHktAutomationTestHarness H;
	H.Setup();

	TMap<uint16, int32> Props;
	Props.Add(PropertyId::Health, 100);
	FHktEntityId Self = H.CreateEntityWithProperties(Props);

	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.BeginPrecondition()
			.LoadStore(Reg::R0, PropertyId::Health)
			.LoadConst(Reg::R1, 50)
			.CmpGt(Reg::Flag, Reg::R0, Reg::R1)  // Health > 50 → Flag=1
			.Halt()
		.EndPrecondition()
		.Halt()
		.Build();

	if (!Program)
		return FHktTestResult::Fail(TEXT("Precondition_ReadOnly"), TEXT("Build failed"));

	FHktEvent TestEvent;
	TestEvent.SourceEntity = Self;

	bool bPass = FHktVMInterpreter::ExecutePrecondition(
		Program->PreconditionCode,
		Program->PreconditionConstants,
		Program->PreconditionStrings,
		H.GetWorldState(),
		TestEvent);

	// WorldState는 변경되지 않아야 함
	int32 HealthAfter = H.GetProperty(Self, PropertyId::Health);
	H.Teardown();

	if (!bPass)
		return FHktTestResult::Fail(TEXT("Precondition_ReadOnly"), TEXT("Precondition should pass (100 > 50)"));
	if (HealthAfter != 100)
		return FHktTestResult::Fail(TEXT("Precondition_ReadOnly"), TEXT("WorldState Health should remain 100"));

	return FHktTestResult::Pass(TEXT("Precondition_ReadOnly"));
}

// ============================================================================
// Edge Cases
// ============================================================================

static FHktTestResult Test_EmptyProgram_AutoHalt()
{
	FHktAutomationTestHarness H;
	H.Setup();
	FHktEntityId E = H.CreateEntity();

	// 빈 Story → Build()에서 자동으로 Halt 추가
	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.Build();

	if (!Program)
		return FHktTestResult::Fail(TEXT("EmptyProgram_AutoHalt"), TEXT("Build should succeed"));

	EVMStatus Status = H.ExecuteProgram(Program, E);
	H.Teardown();

	if (Status != EVMStatus::Completed)
		return FHktTestResult::Fail(TEXT("EmptyProgram_AutoHalt"), TEXT("Empty program should auto-halt and complete"));

	return FHktTestResult::Pass(TEXT("EmptyProgram_AutoHalt"));
}

static FHktTestResult Test_LabelResolution_ForwardBackward()
{
	FHktAutomationTestHarness H;
	H.Setup();
	FHktEntityId E = H.CreateEntity();

	// 전방 점프 + 후방 점프 (카운터 루프)
	auto Program = FHktStoryBuilder::Create(CompositeTestTag())
		.LoadConst(Reg::R0, 0)           // counter = 0
		.LoadConst(Reg::R1, 3)           // limit = 3
		.Label(TEXT("loop"))
		.CmpLt(Reg::R2, Reg::R0, Reg::R1)  // counter < limit?
		.JumpIfNot(Reg::R2, TEXT("done"))    // 전방 점프
		.AddImm(Reg::R0, Reg::R0, 1)
		.Jump(TEXT("loop"))                  // 후방 점프
		.Label(TEXT("done"))
		.Halt()
		.Build();

	H.ExecuteProgram(Program, E);
	H.Teardown();

	if (H.GetRegister(Reg::R0) != 3)
		return FHktTestResult::Fail(TEXT("LabelResolution"), FString::Printf(TEXT("Counter should be 3, got %d"), H.GetRegister(Reg::R0)));

	return FHktTestResult::Pass(TEXT("LabelResolution_ForwardBackward"));
}

// ============================================================================
// Public
// ============================================================================

FHktTestReport RunCompositeTests()
{
	FHktTestReport Report;
	Report.Add(Test_GetPosition_SetPosition());
	Report.Add(Test_MoveToward());
	Report.Add(Test_MoveForward());
	Report.Add(Test_StopMovement());
	Report.Add(Test_ApplyDamage());
	Report.Add(Test_ApplyDamage_ClampZero());
	Report.Add(Test_ApplyDamage_MinDamage());
	Report.Add(Test_ForEachInRadius());
	Report.Add(Test_ForEachInRadius_Empty());
	Report.Add(Test_Precondition_Pass());
	Report.Add(Test_Precondition_Fail());
	Report.Add(Test_Precondition_ReadOnly());
	Report.Add(Test_EmptyProgram_AutoHalt());
	Report.Add(Test_LabelResolution_ForwardBackward());
	return Report;
}

} // namespace HktOpcodeTests
