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
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// Ablackmyth_wukongCharacter

Ablackmyth_wukongCharacter::Ablackmyth_wukongCharacter()
{
	// 允许每帧 Tick
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	WalkSpeed = 500.0f;
	SprintSpeed = 900.0f;
	bIsSprinting = false;

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;

	ComboIndex = 0;
	bIsAttacking = false;
	bIsDodging = false;
	bIsDead = false;

	// [新增] 受击状态初始化
	bIsHitReacting = false;

	bDodgeOnCooldown = false;
	DodgeCooldownTime = 0.5f;
	DodgePlayRate = 1.3f;
	DodgeStrength = 770.0f;

	AttackRange = 150.0f;
	SkillAttackRange = 400.0f;
	AttackRadius = 80.0f;
	bShowHitDebug = true;

	SkillCooldownTime = 5.0f;
	bIsSkillOnCooldown = false;
	CurrentSkillMontage = nullptr;

	CharacterLevel = 1;
	CurrentXP = 0.0f;
	MaxXP = 100.0f;
	BaseAttackPower = 10.0f;

	IdleWaitTime = 10.0f;
	LastInputTime = 0.0;
	CurrentIdleMontage = nullptr;
}

void Ablackmyth_wukongCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

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

	ResetIdleTimer();
}

void Ablackmyth_wukongCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 受击时也不播放 Idle
	if (bIsDead || bIsHitReacting || !IdleAnimSequence) return;

	double CurrentTime = GetWorld()->GetTimeSeconds();
	bool bIsMoving = GetVelocity().SizeSquared() > 10.0f;
	bool bIsFalling = GetCharacterMovement()->IsFalling();

	if (bIsMoving || bIsFalling)
	{
		LastInputTime = CurrentTime;
		return;
	}

	if ((CurrentTime - LastInputTime) > IdleWaitTime)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			if (!AnimInstance->IsAnyMontagePlaying())
			{
				CurrentIdleMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(
					IdleAnimSequence,
					FName("DefaultSlot"),
					0.25f, 0.25f, 1.0f, 1
				);
				UE_LOG(LogTemplateCharacter, Log, TEXT("Player AFK detected. Playing Idle Animation Sequence."));
			}
		}
	}
}

void Ablackmyth_wukongCharacter::ResetIdleTimer()
{
	if (GetWorld())
	{
		LastInputTime = GetWorld()->GetTimeSeconds();
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CurrentIdleMontage)
	{
		if (AnimInstance->Montage_IsPlaying(CurrentIdleMontage))
		{
			AnimInstance->Montage_Stop(0.25f, CurrentIdleMontage);
		}
		CurrentIdleMontage = nullptr;
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

		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::Sprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &Ablackmyth_wukongCharacter::StopSprinting);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &Ablackmyth_wukongCharacter::StopSprinting);
		}

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

		if (SpecialSkillAction)
		{
			EnhancedInputComponent->BindAction(SpecialSkillAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformSpecialSkill);
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
	}
}

void Ablackmyth_wukongCharacter::Move(const FInputActionValue& Value)
{
	ResetIdleTimer();

	// [修改] 增加 bIsHitReacting，受击时不可移动 (硬直)
	if (bIsDead || bIsDodging || bIsAttacking || bIsHitReacting) return;

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
	ResetIdleTimer();

	if (bIsDead) return;

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

// -------------------------------------------------------------------------
// [奔跑系统]
// -------------------------------------------------------------------------

void Ablackmyth_wukongCharacter::Sprint()
{
	ResetIdleTimer();

	// [修改] 受击时不可奔跑
	if (bIsDead || bIsDodging || bIsAttacking || bIsHitReacting) return;

	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

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

	if (GetMesh())
	{
		GetMesh()->GlobalAnimRateScale = 1.0f;
	}
}

// -------------------------------------------------------------------------
// [攻击判定]
// -------------------------------------------------------------------------

void Ablackmyth_wukongCharacter::CheckAttackHit(float CurrentRange)
{
	if (bIsDead) return;

	FVector Start = GetActorLocation();
	FVector End = Start + (GetActorForwardVector() * CurrentRange);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	TArray<FHitResult> OutHits;

	bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
		GetWorld(),
		Start,
		End,
		AttackRadius,
		ObjectTypes,
		false,
		ActorsToIgnore,
		bShowHitDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		OutHits,
		true
	);

	if (bHit)
	{
		TSet<AActor*> HitActors;

		for (const FHitResult& Hit : OutHits)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && !HitActors.Contains(HitActor))
			{
				HitActors.Add(HitActor);

				float Damage = GetTotalAttackPower();

				UGameplayStatics::ApplyDamage(
					HitActor,
					Damage,
					GetController(),
					this,
					UDamageType::StaticClass()
				);

				UE_LOG(LogTemplateCharacter, Log, TEXT("Hit Enemy: %s, Damage: %f"), *HitActor->GetName(), Damage);
			}
		}
	}
}

// -------------------------------------------------------------------------
// [战斗系统]
// -------------------------------------------------------------------------

void Ablackmyth_wukongCharacter::PerformLightAttack(const FInputActionValue& Value)
{
	ResetIdleTimer();

	// [修改] 受击时不可攻击
	if (bIsDead || bIsDodging || bIsHitReacting) return;
	if (LightAttackMontages.Num() == 0) return;

	StopSprinting();

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	if (HeavyAttackMontage && AnimInstance->Montage_IsPlaying(HeavyAttackMontage)) return;
	if (CurrentSkillMontage && AnimInstance->Montage_IsPlaying(CurrentSkillMontage)) return;

	if (ComboIndex >= LightAttackMontages.Num()) ComboIndex = 0;

	UAnimMontage* MontageToPlay = LightAttackMontages[ComboIndex];

	if (MontageToPlay)
	{
		AnimInstance->Montage_Play(MontageToPlay);

		bIsAttacking = true;
		ComboIndex++;

		// 使用 Lambda 传递 AttackRange
		FTimerHandle HitCheckTimer;
		GetWorldTimerManager().SetTimer(HitCheckTimer, [this]()
			{
				CheckAttackHit(AttackRange);
			}, 0.2f, false);

		GetWorldTimerManager().ClearTimer(ComboResetTimer);
		GetWorldTimerManager().SetTimer(ComboResetTimer, this, &Ablackmyth_wukongCharacter::ResetCombo, 1.2f, false);
	}
}

void Ablackmyth_wukongCharacter::PerformHeavyAttack(const FInputActionValue& Value)
{
	ResetIdleTimer();

	if (bIsDead || bIsDodging || bIsHitReacting || !HeavyAttackMontage) return;

	StopSprinting();

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	if (CurrentSkillMontage && AnimInstance->Montage_IsPlaying(CurrentSkillMontage)) return;

	if (!AnimInstance->Montage_IsPlaying(HeavyAttackMontage))
	{
		AnimInstance->StopAllMontages(0.2f);
		AnimInstance->Montage_Play(HeavyAttackMontage);

		FTimerHandle HitCheckTimer;
		GetWorldTimerManager().SetTimer(HitCheckTimer, [this]()
			{
				CheckAttackHit(AttackRange);
			}, 0.4f, false);

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
	ResetIdleTimer();

	// [修改] 受击时不可闪避 (除非你想做受击强制取消，这里默认是硬直状态不可闪避)
	if (bIsDead || bIsDodging || bIsHitReacting || bDodgeOnCooldown || !DodgeAnimSequence) return;

	StopSprinting();

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

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

	ResetCombo();
	AnimInstance->StopAllMontages(0.1f);

	AnimInstance->PlaySlotAnimationAsDynamicMontage(
		DodgeAnimSequence,
		FName("DefaultSlot"),
		0.1f, 0.2f, DodgePlayRate, 1);

	bIsDodging = true;
	bDodgeOnCooldown = true;

	float AnimDuration = DodgeAnimSequence->GetPlayLength() / DodgePlayRate;

	GetWorldTimerManager().SetTimer(DodgeResetTimer, this, &Ablackmyth_wukongCharacter::ResetDodgeState, AnimDuration, false);
	GetWorldTimerManager().SetTimer(DodgeCooldownTimer, this, &Ablackmyth_wukongCharacter::ResetDodgeCooldown, DodgeCooldownTime, false);

	bool bIsFalling = GetCharacterMovement()->IsFalling();

	if (bIsFalling)
	{
		GetCharacterMovement()->BrakingDecelerationFalling = 0.0f;
		LaunchCharacter(FinalDodgeDir * DodgeStrength, true, false);
	}
	else
	{
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->GroundFriction = 0.0f;
		GetCharacterMovement()->BrakingDecelerationWalking = 0.0f;
		LaunchCharacter(FinalDodgeDir * DodgeStrength, true, true);
	}
}

void Ablackmyth_wukongCharacter::ResetDodgeState()
{
	if (bIsDead) return;

	bIsDodging = false;

	GetCharacterMovement()->GroundFriction = 8.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
}

void Ablackmyth_wukongCharacter::ResetDodgeCooldown()
{
	bDodgeOnCooldown = false;
}

// -------------------------------------------------------------------------
// [特殊技能系统]
// -------------------------------------------------------------------------

void Ablackmyth_wukongCharacter::PerformSpecialSkill(const FInputActionValue& Value)
{
	if (bIsDead || bIsSkillOnCooldown || bIsDodging || bIsHitReacting)
	{
		return;
	}

	if (!SpecialSkillAnimSequence)
	{
		return;
	}

	ResetIdleTimer();
	StopSprinting();
	GetCharacterMovement()->StopMovementImmediately();

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->StopAllMontages(0.2f);

		CurrentSkillMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(
			SpecialSkillAnimSequence,
			FName("DefaultSlot"),
			0.2f, 0.2f, 1.0f, 1
		);

		ResetCombo();
		bIsAttacking = true;

		FTimerHandle HitCheckTimer;
		GetWorldTimerManager().SetTimer(HitCheckTimer, [this]()
			{
				CheckAttackHit(SkillAttackRange);
			}, 0.3f, false);

		float AnimLength = SpecialSkillAnimSequence->GetPlayLength();
		FTimerHandle SkillAnimTimer;
		GetWorldTimerManager().SetTimer(SkillAnimTimer, [this]()
			{
				if (!bIsDead && !bIsDodging && !bIsHitReacting)
				{
					bIsAttacking = false;
					CurrentSkillMontage = nullptr;
				}
			}, AnimLength, false);
	}

	bIsSkillOnCooldown = true;
	GetWorldTimerManager().SetTimer(SkillCooldownTimer, this, &Ablackmyth_wukongCharacter::ResetSkillCooldown, SkillCooldownTime, false);

	UE_LOG(LogTemplateCharacter, Log, TEXT("Special Skill Used! Cooldown: %f"), SkillCooldownTime);
}

void Ablackmyth_wukongCharacter::ResetSkillCooldown()
{
	bIsSkillOnCooldown = false;
	UE_LOG(LogTemplateCharacter, Log, TEXT("Special Skill Ready!"));
}

float Ablackmyth_wukongCharacter::GetSkillCooldownFraction() const
{
	if (GetWorldTimerManager().IsTimerActive(SkillCooldownTimer))
	{
		float Remaining = GetWorldTimerManager().GetTimerRemaining(SkillCooldownTimer);
		float Total = SkillCooldownTime > 0.f ? SkillCooldownTime : 1.f;
		return FMath::Clamp(Remaining / Total, 0.0f, 1.0f);
	}
	return 0.0f;
}

// -------------------------------------------------------------------------
// [RPG/Damage 系统]
// -------------------------------------------------------------------------

float Ablackmyth_wukongCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.0f;

	ResetIdleTimer();

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
	UE_LOG(LogTemplateCharacter, Warning, TEXT("Took Damage: %f, Health: %f"), ActualDamage, CurrentHealth);

	if (CurrentHealth <= 0.0f)
	{
		Die();
	}
	else if (HitReactAnimSequence)
	{
		// [核心新增] 如果没死，且有受击动画，则播放受击动画 (Hit Stun)

		// 1. 打断当前所有动作 (奔跑、攻击、技能)
		StopSprinting();
		GetCharacterMovement()->StopMovementImmediately();
		ResetCombo();

		// 2. 播放受击动画 (作为 Dynamic Montage)
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			// 0.1s 快速混合，打断当前动作
			AnimInstance->StopAllMontages(0.1f);
			AnimInstance->PlaySlotAnimationAsDynamicMontage(
				HitReactAnimSequence,
				FName("DefaultSlot"),
				0.1f, 0.1f, 1.0f, 1
			);
		}

		// 3. 设置受击状态 (锁定输入)
		bIsHitReacting = true;

		// 4. 设置计时器恢复状态
		float AnimLength = HitReactAnimSequence->GetPlayLength();

		// 清除之前的恢复计时器 (防止连续受击逻辑错乱)
		GetWorldTimerManager().ClearTimer(HitReactResetTimer);
		GetWorldTimerManager().SetTimer(HitReactResetTimer, this, &Ablackmyth_wukongCharacter::ResetHitReactState, AnimLength, false);
	}

	return ActualDamage;
}

void Ablackmyth_wukongCharacter::ResetHitReactState()
{
	// 恢复行动能力
	bIsHitReacting = false;
}

void Ablackmyth_wukongCharacter::Die()
{
	if (bIsDead) return;
	bIsDead = true;

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
	GetWorldTimerManager().ClearTimer(SkillCooldownTimer);
	GetWorldTimerManager().ClearTimer(HitReactResetTimer); // [新增] 清理受击计时

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	if (DeathAnimSequence && AnimInstance)
	{
		float AnimLength = DeathAnimSequence->GetPlayLength();
		float BlendOutTime = 0.25f;

		AnimInstance->PlaySlotAnimationAsDynamicMontage(
			DeathAnimSequence,
			FName("DefaultSlot"),
			0.25f, 0.25f, 1.0f, 1
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

void Ablackmyth_wukongCharacter::GainExperience(float Amount)
{
	if (bIsDead) return;
	CurrentXP += Amount;
	UE_LOG(LogTemplateCharacter, Log, TEXT("Gained XP: %f. Total: %f / %f"), Amount, CurrentXP, MaxXP);
	CheckLevelUp();
}

void Ablackmyth_wukongCharacter::CheckLevelUp()
{
	while (CurrentXP >= MaxXP)
	{
		CurrentXP -= MaxXP;
		CharacterLevel++;
		MaxHealth += 20.0f;
		BaseAttackPower += 5.0f;
		MaxXP = MaxXP * 1.5f;
		CurrentHealth = MaxHealth;
		UE_LOG(LogTemplateCharacter, Log, TEXT("Level Up! New Level: %d. Stats Increased."), CharacterLevel);
		OnLevelUp();
	}
}

float Ablackmyth_wukongCharacter::GetTotalAttackPower() const
{
	return BaseAttackPower;
}