// Copyright Hkt Studios, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "HktValidationLog.h"

DEFINE_LOG_CATEGORY(LogHktValidation);

// Forward declarations
namespace HktValidationRunner
{
	void RunAllTests();
	void RunOpcodeTests();
	void RunStoryTests();
	void PrintLastReport();
}

/**
 * HktValidation 모듈 — 자동 테스트 환경
 */
class FHktValidationModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TArray<IConsoleObject*> ConsoleCommands;
};

IMPLEMENT_MODULE(FHktValidationModule, HktValidation)

void FHktValidationModule::StartupModule()
{
	UE_LOG(LogHktValidation, Log, TEXT("[HktValidation] Module starting up"));

	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("hkt.validation.run"),
		TEXT("Run all HktValidation tests (Opcode + Story Integrity)"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			HktValidationRunner::RunAllTests();
		}),
		ECVF_Default));

	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("hkt.validation.opcodes"),
		TEXT("Run Opcode validation tests only"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			HktValidationRunner::RunOpcodeTests();
		}),
		ECVF_Default));

	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("hkt.validation.stories"),
		TEXT("Run Story integrity tests only"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			HktValidationRunner::RunStoryTests();
		}),
		ECVF_Default));

	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("hkt.validation.report"),
		TEXT("Print the last validation report"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			HktValidationRunner::PrintLastReport();
		}),
		ECVF_Default));
}

void FHktValidationModule::ShutdownModule()
{
	for (IConsoleObject* Cmd : ConsoleCommands)
	{
		IConsoleManager::Get().UnregisterConsoleObject(Cmd);
	}
	ConsoleCommands.Empty();

	UE_LOG(LogHktValidation, Log, TEXT("[HktValidation] Module shut down"));
}
