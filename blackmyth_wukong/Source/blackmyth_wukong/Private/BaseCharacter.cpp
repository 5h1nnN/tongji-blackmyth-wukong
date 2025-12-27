#include "BaseCharacter.h"
#include "FeyCharacter.h" // 如果变身逻辑需要识别 Fey
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h" // 必须包含：用于 QuitGame
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	bIsCooldown = false;
	TransformCooldownDuration = 30.0f; // 默认冷却时间，可在蓝图调整
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// --- 输入映射绑定 ---
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// 确保添加默认的 Context (包含变身、暂停等基础功能)
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 绑定变身
		if (TransformAction)
		{
			EnhancedInputComponent->BindAction(TransformAction, ETriggerEvent::Started, this, &ABaseCharacter::TransformCharacter);
		}

		// 绑定暂停
		if (PauseAction)
		{
			EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &ABaseCharacter::TogglePause);
		}
	}
}

// ============================================================================
//                              暂停与 UI 系统 (核心修复)
// ============================================================================

void ABaseCharacter::TogglePause(const FInputActionValue& Value)
{
	APlayerController* PC = Cast<APlayerController>(GetController());

	if (PC && PauseMenuWidgetClass)
	{
		// 检查当前是否已经暂停
		if (UGameplayStatics::IsGamePaused(GetWorld()))
		{
			// --- 恢复游戏 ---
			ResumeGameFromUI();
		}
		else
		{
			// --- 暂停游戏 ---
			UGameplayStatics::SetGamePaused(GetWorld(), true);

			// 创建 UI 实例（如果不存在）
			if (!PauseMenuInstance)
			{
				PauseMenuInstance = CreateWidget<UUserWidget>(GetWorld(), PauseMenuWidgetClass);
			}

			// 显示 UI 并切换输入模式
			if (PauseMenuInstance)
			{
				PauseMenuInstance->AddToViewport();

				// [关键修复] 使用 FInputModeGameAndUI 并设置焦点
				FInputModeGameAndUI InputMode;
				InputMode.SetWidgetToFocus(PauseMenuInstance->TakeWidget()); // 让 UI 立即获得焦点
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

				PC->SetInputMode(InputMode);
				PC->bShowMouseCursor = true;
			}
		}
	}
}

void ABaseCharacter::ResumeGameFromUI()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// 解除暂停状态
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	// 隐藏 UI
	if (PauseMenuInstance)
	{
		PauseMenuInstance->RemoveFromParent();
		// 注意：如果不销毁实例，下次打开会保留上次的状态。
		// 如果希望每次打开都是新的，可以添加 PauseMenuInstance = nullptr;
	}

	// 恢复输入模式为纯游戏
	PC->bShowMouseCursor = false;
	FInputModeGameOnly GameInputMode;
	PC->SetInputMode(GameInputMode);
}

void ABaseCharacter::RestartLevel()
{
	// [必须] 在重新加载关卡前解除暂停，否则新关卡可能卡住
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName));
}

void ABaseCharacter::QuitGame()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, true);
	}
}

// ============================================================================
//                                 变身系统
// ============================================================================

void ABaseCharacter::TransformCharacter()
{
	if (bIsCooldown) return;
	if (!TargetCharacterClass) return;

	UWorld* World = GetWorld();
	if (World)
	{
		FVector Location = GetActorLocation();
		FTransform SpawnTransform = GetActorTransform();

		// 播放特效
		if (TransformationVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, TransformationVFX, Location);
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 生成新角色
		ABaseCharacter* NewCharacter = World->SpawnActor<ABaseCharacter>(TargetCharacterClass, SpawnTransform, SpawnParams);

		if (NewCharacter)
		{
			// 继承血量
			NewCharacter->CurrentHealth = this->CurrentHealth;

			// 如果当前是 Fey (从 Fey 变回 Wukong)，或者新的是 Fey，
			// 根据你的逻辑，这里可能需要触发冷却。
			// 假设逻辑是：只要使用了变身功能，就进入冷却。
			AFeyCharacter* CurrentIsFey = Cast<AFeyCharacter>(this);

			// 控制权移交
			AController* CurrController = GetController();
			APlayerController* PC = Cast<APlayerController>(CurrController);

			if (PC)
			{
				PC->SetViewTargetWithBlend(NewCharacter, 0.2f);
			}

			if (CurrController)
			{
				CurrController->Possess(NewCharacter);
			}

			// 如果是从 Fey 变回 Wukong (或者反过来)，在新角色身上启动冷却逻辑
			// 这里假设是 NewCharacter 需要继承或启动冷却管理
			if (CurrentIsFey)
			{
				NewCharacter->StartTransformCooldown();
			}

			// 销毁旧角色
			Destroy();
		}
	}
}

void ABaseCharacter::StartTransformCooldown()
{
	bIsCooldown = true;
	GetWorldTimerManager().SetTimer(CooldownTimerHandle, this, &ABaseCharacter::OnCooldownFinished, TransformCooldownDuration, false);
}

void ABaseCharacter::OnCooldownFinished()
{
	bIsCooldown = false;
	GetWorldTimerManager().ClearTimer(CooldownTimerHandle);
}

float ABaseCharacter::GetCooldownPercent() const
{
	if (bIsCooldown && GetWorldTimerManager().IsTimerActive(CooldownTimerHandle))
	{
		return FMath::Clamp(GetWorldTimerManager().GetTimerRemaining(CooldownTimerHandle) / TransformCooldownDuration, 0.0f, 1.0f);
	}
	return 0.0f;
}