// Copyright Epic Games, Inc. All Rights Reserved.

#include "blackmyth_wukongCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

// 必须引用的头文件
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h" 
#include "Blueprint/UserWidget.h" 
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// Ablackmyth_wukongCharacter

Ablackmyth_wukongCharacter::Ablackmyth_wukongCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;

	// [默认参数] 正常行走的刹车阻尼
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom 
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// [初始化] 血量
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;

	// [初始化] 战斗状态
	ComboIndex = 0;
	bIsAttacking = false;
	bIsDodging = false;
	bIsDead = false;

	// [初始化] 冷却参数
	bDodgeOnCooldown = false;
	DodgeCooldownTime = 0.5f;
	DodgePlayRate = 1.3f; // 建议根据动画实际速度微调，越快滑动时间越短

	// [修改] 提高默认冲刺力度
	// 由于我们现在加大了闪避时的阻尼(8000)，需要很大的力才能推出去
	DodgeStrength = 770.0f;
}

void Ablackmyth_wukongCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void Ablackmyth_wukongCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &Ablackmyth_wukongCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &Ablackmyth_wukongCharacter::Look);

		// 战斗绑定
		if (LightAttackAction)
		{
			EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformLightAttack);
		}
		if (HeavyAttackAction)
		{
			EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformHeavyAttack);
		}

		if (DodgeAction)
		{
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformDodge);
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
	}
}

void Ablackmyth_wukongCharacter::Move(const FInputActionValue& Value)
{
	// 死亡或闪避中不可移动
	if (bIsDead || bIsDodging) return;

	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void Ablackmyth_wukongCharacter::Look(const FInputActionValue& Value)
{
	if (bIsDead) return;

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

// -------------------------------------------------------------------------
// [战斗系统]
// -------------------------------------------------------------------------

void Ablackmyth_wukongCharacter::PerformLightAttack(const FInputActionValue& Value)
{
	if (bIsDead || bIsDodging) return;
	if (LightAttackMontages.Num() == 0) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	if (HeavyAttackMontage && AnimInstance->Montage_IsPlaying(HeavyAttackMontage)) return;

	if (ComboIndex >= LightAttackMontages.Num()) ComboIndex = 0;

	UAnimMontage* MontageToPlay = LightAttackMontages[ComboIndex];

	if (MontageToPlay)
	{
		AnimInstance->Montage_Play(MontageToPlay);
		bIsAttacking = true;
		ComboIndex++;

		GetWorldTimerManager().ClearTimer(ComboResetTimer);
		GetWorldTimerManager().SetTimer(ComboResetTimer, this, &Ablackmyth_wukongCharacter::ResetCombo, 1.2f, false);
	}
}

void Ablackmyth_wukongCharacter::PerformHeavyAttack(const FInputActionValue& Value)
{
	if (bIsDead || bIsDodging || !HeavyAttackMontage) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	if (!AnimInstance->Montage_IsPlaying(HeavyAttackMontage))
	{
		AnimInstance->StopAllMontages(0.2f);
		AnimInstance->Montage_Play(HeavyAttackMontage);
		ResetCombo();
	}
}

void Ablackmyth_wukongCharacter::ResetCombo()
{
	ComboIndex = 0;
	bIsAttacking = false;
}

// [修改] 完美匹配动画时长与物理滑行
void Ablackmyth_wukongCharacter::PerformDodge(const FInputActionValue& Value)
{
	if (bIsDead || bIsDodging || bDodgeOnCooldown || !DodgeAnimSequence) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	// --- 1. 计算闪避方向 ---
	FVector FinalDodgeDir;
	FVector InputDir = GetLastMovementInputVector();

	if (InputDir.IsNearlyZero())
	{
		// 无输入则后撤
		FinalDodgeDir = GetActorForwardVector();
	}
	else
	{
		// 有输入则按输入方向
		FinalDodgeDir = InputDir.GetSafeNormal();
	}

	// --- 2. 物理状态重置 (关键步骤) ---
	// 彻底清除之前的跑步速度，避免惯性叠加
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	GetCharacterMovement()->StopMovementImmediately();

	// [核心修改 A] 临时大幅提高刹车阻尼
	// 让角色在冲刺后能迅速停下，而不是像冰面一样滑很远
	// 8000.0f 是一个比较强的值，配合 4000.0f 的 DodgeStrength
	//GetCharacterMovement()->BrakingDecelerationWalking = 0.0f;
	// 坏文明：消除了摩擦力，导致像在溜冰
	GetCharacterMovement()->GroundFriction = 0.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 0.0f;

	// --- 3. 播放动画 ---
	ResetCombo();
	AnimInstance->StopAllMontages(0.1f);
	AnimInstance->PlaySlotAnimationAsDynamicMontage(
		DodgeAnimSequence,
		FName("DefaultSlot"),
		0.1f, 0.2f, DodgePlayRate, 1);

	// --- 4. 设置状态与计时器 ---
	bIsDodging = true;
	bDodgeOnCooldown = true;

	float AnimDuration = DodgeAnimSequence->GetPlayLength() / DodgePlayRate;

	// 动画结束时调用 ResetDodgeState
	GetWorldTimerManager().SetTimer(DodgeResetTimer, this, &Ablackmyth_wukongCharacter::ResetDodgeState, AnimDuration, false);
	GetWorldTimerManager().SetTimer(DodgeCooldownTimer, this, &Ablackmyth_wukongCharacter::ResetDodgeCooldown, DodgeCooldownTime, false);

	// --- 5. 施加爆发力 ---
	// 使用覆盖模式 (Override)，确保力道准确
	LaunchCharacter(FinalDodgeDir * DodgeStrength, true, true);
}

void Ablackmyth_wukongCharacter::ResetDodgeState()
{
	if (bIsDead) return;

	bIsDodging = false;

	// [核心修改 B] 动画结束瞬间，强制刹车
	// 确保"动画停，脚就停"，解决滑步过头的问题
	GetCharacterMovement()->StopMovementImmediately();

	// [核心修改 C] 恢复正常的刹车阻尼
	// 恢复成构造函数里设置的 2000.0f，保证后续正常走路手感
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
}

void Ablackmyth_wukongCharacter::ResetDodgeCooldown()
{
	bDodgeOnCooldown = false;
}

float Ablackmyth_wukongCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.0f;

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
	UE_LOG(LogTemplateCharacter, Warning, TEXT("Took Damage: %f, Health: %f"), ActualDamage, CurrentHealth);

	if (CurrentHealth <= 0.0f)
	{
		Die();
	}

	return ActualDamage;
}

void Ablackmyth_wukongCharacter::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->StopAllMontages(0.2f);
	}
	ResetCombo();

	GetWorldTimerManager().ClearTimer(DodgeResetTimer);
	GetWorldTimerManager().ClearTimer(DodgeCooldownTimer);
	GetWorldTimerManager().ClearTimer(ComboResetTimer);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	if (DeathAnimSequence && AnimInstance)
	{
		float AnimLength = DeathAnimSequence->GetPlayLength();
		float BlendOutTime = 0.25f;

		AnimInstance->PlaySlotAnimationAsDynamicMontage(
			DeathAnimSequence,
			FName("DefaultSlot"),
			0.25f,
			BlendOutTime,
			1.0f,
			1
		);

		float RagdollDelay = FMath::Max(0.0f, AnimLength - BlendOutTime);

		FTimerHandle TimerHandle_Ragdoll;
		GetWorldTimerManager().SetTimer(TimerHandle_Ragdoll, [this]()
			{
				GetMesh()->SetSimulatePhysics(true);
				GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
				GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				GetMesh()->bPauseAnims = true;
			}, RagdollDelay, false);
	}
	else
	{
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FTimerHandle TimerHandle_ShowUI;
		GetWorldTimerManager().SetTimer(TimerHandle_ShowUI, [this, PC]()
			{
				if (GameOverWidgetClass)
				{
					UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), GameOverWidgetClass);
					if (Widget)
					{
						Widget->AddToViewport();
						PC->bShowMouseCursor = true;
						FInputModeUIOnly InputMode;
						InputMode.SetWidgetToFocus(Widget->TakeWidget());
						PC->SetInputMode(InputMode);
					}
				}
			}, 2.0f, false);
	}
}