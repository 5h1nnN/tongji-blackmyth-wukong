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
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;

	// [默认参数] 正常行走的刹车阻尼
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// [新增] 速度配置初始化
	WalkSpeed = 500.0f;
	SprintSpeed = 900.0f;
	bIsSprinting = false;

	// 应用默认速度
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

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
	DodgePlayRate = 1.3f;

	// [修改] 提高默认冲刺力度
	DodgeStrength = 770.0f;
}

void Ablackmyth_wukongCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	// 确保开始游戏时速度正确
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// 确保动画速率正常
	if (GetMesh())
	{
		GetMesh()->GlobalAnimRateScale = 1.0f;
	}

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

		// [新增] 奔跑绑定
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::Sprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &Ablackmyth_wukongCharacter::StopSprinting);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &Ablackmyth_wukongCharacter::StopSprinting);
		}

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
// [新增] 奔跑系统
// -------------------------------------------------------------------------

void Ablackmyth_wukongCharacter::Sprint()
{
	// 死亡、闪避中、攻击中、或者正在坠落时不允许开启奔跑
	if (bIsDead || bIsDodging || bIsAttacking) return;

	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

	// --- [核心修改] 纯C++控制动画速率 ---
	// 计算加速比率 (例如 800/500 = 1.6倍)
	// 加上 KINDA_SMALL_NUMBER 防止除以0 (虽然构造函数里已经赋值了)
	float SpeedRatio = SprintSpeed / (WalkSpeed > KINDA_SMALL_NUMBER ? WalkSpeed : 500.0f);

	if (GetMesh())
	{
		GetMesh()->GlobalAnimRateScale = SpeedRatio;
	}
}

void Ablackmyth_wukongCharacter::StopSprinting()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// --- [核心修改] 恢复正常动画速率 ---
	if (GetMesh())
	{
		GetMesh()->GlobalAnimRateScale = 1.0f;
	}
}

// -------------------------------------------------------------------------
// [战斗系统]
// -------------------------------------------------------------------------

void Ablackmyth_wukongCharacter::PerformLightAttack(const FInputActionValue& Value)
{
	if (bIsDead || bIsDodging) return;
	if (LightAttackMontages.Num() == 0) return;

	// 攻击时强制停止奔跑 (这会自动调用 StopSprinting 恢复动画速率为 1.0)
	StopSprinting();

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

	// 攻击时强制停止奔跑
	StopSprinting();

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

void Ablackmyth_wukongCharacter::PerformDodge(const FInputActionValue& Value)
{
	if (bIsDead || bIsDodging || bDodgeOnCooldown || !DodgeAnimSequence) return;

	// 闪避时停止奔跑状态
	StopSprinting();

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	// --- 1. 计算闪避方向 ---
	FVector InputDir = GetLastMovementInputVector();
	FVector FinalDodgeDir;

	if (InputDir.IsNearlyZero())
	{
		FinalDodgeDir = GetActorForwardVector();
	}
	else
	{
		FinalDodgeDir = InputDir.GetSafeNormal();
	}

	// --- 2. 物理状态重置 (修复空中定身 BUG) ---
	bool bIsFalling = GetCharacterMovement()->IsFalling();

	if (bIsFalling)
	{
		FVector CurrentVel = GetCharacterMovement()->Velocity;
		GetCharacterMovement()->Velocity = FVector(0.f, 0.f, CurrentVel.Z);
	}
	else
	{
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
		GetCharacterMovement()->StopMovementImmediately();

		GetCharacterMovement()->GroundFriction = 0.0f;
		GetCharacterMovement()->BrakingDecelerationWalking = 0.0f;
	}

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

	GetWorldTimerManager().SetTimer(DodgeResetTimer, this, &Ablackmyth_wukongCharacter::ResetDodgeState, AnimDuration, false);
	GetWorldTimerManager().SetTimer(DodgeCooldownTimer, this, &Ablackmyth_wukongCharacter::ResetDodgeCooldown, DodgeCooldownTime, false);

	// --- 5. 施加爆发力 ---
	LaunchCharacter(FinalDodgeDir * DodgeStrength, true, true);
}

void Ablackmyth_wukongCharacter::ResetDodgeState()
{
	if (bIsDead) return;

	bIsDodging = false;

	// [核心修复] 恢复正常的物理参数
	GetCharacterMovement()->GroundFriction = 8.0f;
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

	// 死亡停止一切动作
	StopSprinting();

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