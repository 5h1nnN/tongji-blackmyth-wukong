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
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/UserWidget.h"

// [新增] 导航系统相关头文件
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "Components/BrushComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

Ablackmyth_wukongCharacter::Ablackmyth_wukongCharacter()
{
	// 设置胶囊体大小
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// 禁止控制器旋转影响角色（只影响相机）
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false; // [关键] 设为 false，角色将始终朝向准星/摄像机前方 (在Tick中处理或Movement中处理)
	// 注意：如果你之前的设置是 true，请根据你的需求保留。标准第三人称通常 Yaw 是 true 或者 MovementOrient 是 true
	// 这里参考你之前的设置：
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// 配置角色移动
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// 创建相机吊臂
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// 创建跟随相机
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 初始化参数
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	WalkSpeed = 500.0f;
	SprintSpeed = 900.0f;
	bIsSprinting = false;
	bIsDead = false;

	DodgeCooldownTime = 0.5f;
	DodgePlayRate = 1.3f;
	DodgeStrength = 770.0f;

	// [新增] 分身默认参数
	CloneCount = 3;
	CloneSpawnRadius = 300.0f;
	CloneLifeSpan = 15.0f;
	CloneSkillCooldown = 30.0f;
	bAutoSpawnNavMesh = true; // 默认开启自动生成导航

	SkillCooldownTime = 5.0f;
	CharacterLevel = 1;
	BaseAttackPower = 10.0f;
	IdleWaitTime = 10.0f;
}

void Ablackmyth_wukongCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// 添加输入映射上下文
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}

		// 视角限制
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->ViewPitchMin = -40.0f;
			PlayerController->PlayerCameraManager->ViewPitchMax = 40.0f;
		}
	}

	ResetIdleTimer();

	// [核心] 尝试生成动态导航网格
	SpawnDynamicNavMesh();
}

// =================================================================
// [核心逻辑] 自动生成 NavMeshBoundsVolume
// =================================================================
void Ablackmyth_wukongCharacter::SpawnDynamicNavMesh()
{
	if (!bAutoSpawnNavMesh) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 检查是否已经存在 (避免重复生成)
	TArray<AActor*> FoundVolumes;
	UGameplayStatics::GetAllActorsOfClass(World, ANavMeshBoundsVolume::StaticClass(), FoundVolumes);
	if (FoundVolumes.Num() > 0)
	{
		UE_LOG(LogTemplateCharacter, Log, TEXT("场景中已存在 NavMeshBoundsVolume，跳过自动生成。"));
		return;
	}

	// 1. 设置生成参数
	// 将体积中心设为角色当前位置，或者 (0,0,0)
	FVector SpawnLoc = GetActorLocation();
	SpawnLoc.Z = 0; // 强制高度为0，确保覆盖地面

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 2. 生成 Volume Actor
	ANavMeshBoundsVolume* NavVolume = World->SpawnActor<ANavMeshBoundsVolume>(ANavMeshBoundsVolume::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);

	if (NavVolume)
	{
		// 3. 设置体积大小
		// 默认 Volume 是很小的，我们将其放大
		// NavMeshExtent 是 (5000, 5000, 1000)，我们需要除以默认笔刷大小(通常是200)
		FVector NewScale = NavMeshExtent / 100.0f;
		NavVolume->SetActorScale3D(NewScale);

		// 确保不被卸载
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			//NavSys->SetActorKeepInLoadedLevels(NavVolume, true);

			// 4. [关键] 通知导航系统更新并重建
			NavSys->OnNavigationBoundsUpdated(NavVolume);
			// 如果是动态运行时生成，可能还需要强制构建一次
			NavSys->Build();

			UE_LOG(LogTemplateCharacter, Warning, TEXT("已自动生成动态 NavMeshBoundsVolume，大小: %s"), *NavMeshExtent.ToString());
		}
	}
}

// =================================================================
// 输入绑定
// =================================================================
void Ablackmyth_wukongCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent); // 调用父类

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// 基础移动
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &Ablackmyth_wukongCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &Ablackmyth_wukongCharacter::Look);

		// 战斗
		if (LightAttackAction)
			EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformLightAttack);
		if (HeavyAttackAction)
			EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformHeavyAttack);
		if (DodgeAction)
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformDodge);
		if (SprintAction) {
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::Sprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &Ablackmyth_wukongCharacter::StopSprinting);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &Ablackmyth_wukongCharacter::StopSprinting);
		}

		if (SpecialSkillAction)
			EnhancedInputComponent->BindAction(SpecialSkillAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformSpecialSkill);

		// [新增] 绑定分身技能
		if (CloneAction)
		{
			EnhancedInputComponent->BindAction(CloneAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformCloneSkill);
		}

		if (PauseAction)
			EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::TogglePause);
	}
}

// =================================================================
// [核心逻辑] 分身术实现
// =================================================================
void Ablackmyth_wukongCharacter::PerformCloneSkill(const FInputActionValue& Value)
{
	// 1. 状态检查
	if (bIsCloneSkillCooldown || bIsDead || bIsHitReacting || bIsDodging || GetCharacterMovement()->IsFalling()) return;

	ResetIdleTimer();
	StopSprinting();
	GetCharacterMovement()->StopMovementImmediately();

	// 清理旧分身
	DestroyClones();

	// 2. 播放动作
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (CloneSummonMontage && AnimInstance)
	{
		AnimInstance->Montage_Play(CloneSummonMontage);
	}

	// 3. 准备生成参数
	UClass* SpawnClass = CloneClass ? CloneClass.Get() : GetClass();
	if (!SpawnClass) return;

	FVector CenterLoc = GetActorLocation();
	FRotator SpawnRot = GetActorRotation();
	float AngleStep = 360.0f / CloneCount;

	// 4. 循环生成
	for (int32 i = 0; i < CloneCount; i++)
	{
		float CurrentAngle = i * AngleStep;
		float Rad = FMath::DegreesToRadians(CurrentAngle);

		// 计算位置
		FVector Offset(FMath::Cos(Rad) * CloneSpawnRadius, FMath::Sin(Rad) * CloneSpawnRadius, 0.0f);
		FVector PotentialLoc = CenterLoc + Offset;

		// 地面检测
		FVector TraceStart = PotentialLoc + FVector(0, 0, 200.0f);
		FVector TraceEnd = PotentialLoc - FVector(0, 0, 200.0f);
		FHitResult HitResult;
		FVector FinalLoc = PotentialLoc;

		if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility))
		{
			FinalLoc = HitResult.Location + FVector(0, 0, 10.0f);
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnParams.Owner = this; // [关键] 设置Owner为本体，用于识别敌我

		// 生成 Actor
		ACharacter* NewClone = GetWorld()->SpawnActor<ACharacter>(SpawnClass, FinalLoc, SpawnRot, SpawnParams);
		if (NewClone)
		{
			// [关键] 赋予 AI 控制器
			NewClone->SpawnDefaultController();

			// [关键] 添加标签 (Clone)，防止被友军误伤
			NewClone->Tags.Add(FName("Clone"));

			// 播放特效
			if (CloneSpawnFX)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), CloneSpawnFX, FinalLoc);
			}

			ActiveClones.Add(NewClone);
		}
	}

	// 5. 设置冷却
	bIsCloneSkillCooldown = true;
	GetWorldTimerManager().SetTimer(CloneCooldownTimer, this, &Ablackmyth_wukongCharacter::ResetCloneSkillCooldown, CloneSkillCooldown, false);
	GetWorldTimerManager().SetTimer(CloneLifeTimer, this, &Ablackmyth_wukongCharacter::DestroyClones, CloneLifeSpan, false);
}

void Ablackmyth_wukongCharacter::DestroyClones()
{
	for (ACharacter* Clone : ActiveClones)
	{
		if (Clone && IsValid(Clone))
		{
			if (CloneSpawnFX)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), CloneSpawnFX, Clone->GetActorLocation());
			}
			Clone->Destroy();
		}
	}
	ActiveClones.Empty();
}

void Ablackmyth_wukongCharacter::ResetCloneSkillCooldown()
{
	bIsCloneSkillCooldown = false;
}

float Ablackmyth_wukongCharacter::GetCloneCooldownFraction() const
{
	if (bIsCloneSkillCooldown && GetWorldTimerManager().IsTimerActive(CloneCooldownTimer))
	{
		float Remaining = GetWorldTimerManager().GetTimerRemaining(CloneCooldownTimer);
		float Total = CloneSkillCooldown > 0.f ? CloneSkillCooldown : 1.f;
		return FMath::Clamp(Remaining / Total, 0.0f, 1.0f);
	}
	return 0.0f;
}

// =================================================================
// 其他逻辑实现
// =================================================================

void Ablackmyth_wukongCharacter::Move(const FInputActionValue& Value)
{
	ResetIdleTimer();
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

void Ablackmyth_wukongCharacter::PerformLightAttack(const FInputActionValue& Value)
{
	ResetIdleTimer();
	if (bIsDead || bIsDodging || bIsHitReacting) return;
	if (LightAttackMontages.Num() == 0) return;
	StopSprinting();
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;
	if (HeavyAttackMontage && AnimInstance->Montage_IsPlaying(HeavyAttackMontage)) return;
	if (CurrentSkillMontage && AnimInstance->Montage_IsPlaying(CurrentSkillMontage)) return;
	if (CloneSummonMontage && AnimInstance->Montage_IsPlaying(CloneSummonMontage)) return;

	if (ComboIndex >= LightAttackMontages.Num()) ComboIndex = 0;
	UAnimMontage* MontageToPlay = LightAttackMontages[ComboIndex];
	if (MontageToPlay)
	{
		AnimInstance->Montage_Play(MontageToPlay);
		bIsAttacking = true;
		ComboIndex++;
		FTimerHandle HitCheckTimer;
		GetWorldTimerManager().SetTimer(HitCheckTimer, [this]() { CheckAttackHit(150.0f); }, 0.2f, false);
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
		GetWorldTimerManager().SetTimer(HitCheckTimer, [this]() { CheckAttackHit(150.0f); }, 0.4f, false);
		ResetCombo();
	}
}

void Ablackmyth_wukongCharacter::PerformDodge(const FInputActionValue& Value)
{
	ResetIdleTimer();
	if (bIsDead || bIsDodging || bIsHitReacting || bDodgeOnCooldown || !DodgeAnimSequence) return;
	StopSprinting();
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;
	FVector InputDir = GetLastMovementInputVector();
	FVector FinalDodgeDir = InputDir.IsNearlyZero() ? GetActorForwardVector() : InputDir.GetSafeNormal();
	ResetCombo();
	AnimInstance->StopAllMontages(0.1f);
	AnimInstance->PlaySlotAnimationAsDynamicMontage(DodgeAnimSequence, FName("DefaultSlot"), 0.1f, 0.2f, DodgePlayRate, 1);
	bIsDodging = true;
	bDodgeOnCooldown = true;
	float AnimDuration = DodgeAnimSequence->GetPlayLength() / DodgePlayRate;
	GetWorldTimerManager().SetTimer(DodgeResetTimer, this, &Ablackmyth_wukongCharacter::ResetDodgeState, AnimDuration, false);
	GetWorldTimerManager().SetTimer(DodgeCooldownTimer, this, &Ablackmyth_wukongCharacter::ResetDodgeCooldown, DodgeCooldownTime, false);
	if (GetCharacterMovement()->IsFalling()) {
		GetCharacterMovement()->BrakingDecelerationFalling = 0.0f;
		LaunchCharacter(FinalDodgeDir * DodgeStrength, true, false);
	}
	else {
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->GroundFriction = 0.0f;
		GetCharacterMovement()->BrakingDecelerationWalking = 0.0f;
		LaunchCharacter(FinalDodgeDir * DodgeStrength, true, true);
	}
}

void Ablackmyth_wukongCharacter::PerformSpecialSkill(const FInputActionValue& Value)
{
	if (bIsDead || bIsSkillOnCooldown || bIsDodging || bIsHitReacting) return;
	if (!SpecialSkillAnimSequence) return;
	ResetIdleTimer();
	StopSprinting();
	GetCharacterMovement()->StopMovementImmediately();
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->StopAllMontages(0.2f);
		CurrentSkillMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(SpecialSkillAnimSequence, FName("DefaultSlot"), 0.2f, 0.2f, 1.0f, 1);
		ResetCombo();
		bIsAttacking = true;
		FTimerHandle HitCheckTimer;
		GetWorldTimerManager().SetTimer(HitCheckTimer, [this]() { CheckAttackHit(400.0f); }, 0.3f, false);
		float AnimLength = SpecialSkillAnimSequence->GetPlayLength();
		FTimerHandle SkillAnimTimer;
		GetWorldTimerManager().SetTimer(SkillAnimTimer, [this]()
			{
				if (!bIsDead && !bIsDodging && !bIsHitReacting) {
					bIsAttacking = false;
					CurrentSkillMontage = nullptr;
				}
			}, AnimLength, false);
	}
	bIsSkillOnCooldown = true;
	GetWorldTimerManager().SetTimer(SkillCooldownTimer, this, &Ablackmyth_wukongCharacter::ResetSkillCooldown, SkillCooldownTime, false);
}

void Ablackmyth_wukongCharacter::ResetCombo() { ComboIndex = 0; bIsAttacking = false; }
void Ablackmyth_wukongCharacter::ResetDodgeCooldown() { bDodgeOnCooldown = false; }
void Ablackmyth_wukongCharacter::ResetDodgeState()
{
	if (bIsDead) return;
	bIsDodging = false;
	GetCharacterMovement()->GroundFriction = 8.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
}
void Ablackmyth_wukongCharacter::ResetSkillCooldown() { bIsSkillOnCooldown = false; }
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
	ActorsToIgnore.Add(this); // 忽略自己

	// [新增] 逻辑：如果我有 Owner (说明我是分身)，我也要忽略我的 Owner
	if (GetOwner())
	{
		ActorsToIgnore.Add(GetOwner());
	}

	// [新增] 逻辑：如果我是本体，忽略我的所有分身
	for (ACharacter* Clone : ActiveClones)
	{
		if (Clone) ActorsToIgnore.Add(Clone);
	}

	// [新增] 逻辑：尝试忽略所有其他的友军分身 (通过 Tag 判断)
	// 防止分身之间互殴
	TArray<AActor*> AllClones;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Clone"), AllClones);
	ActorsToIgnore.Append(AllClones);

	TArray<FHitResult> OutHits;
	bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(GetWorld(), Start, End, 80.0f, ObjectTypes, false, ActorsToIgnore, EDrawDebugTrace::None, OutHits, true);

	if (bHit)
	{
		TSet<AActor*> HitActors;
		for (const FHitResult& Hit : OutHits)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && !HitActors.Contains(HitActor))
			{
				HitActors.Add(HitActor);
				// 造成伤害
				UGameplayStatics::ApplyDamage(HitActor, GetTotalAttackPower(), GetController(), this, UDamageType::StaticClass());
			}
		}
	}
}

float Ablackmyth_wukongCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.0f;
	ResetIdleTimer();
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
	if (CurrentHealth <= 0.0f) Die();
	else if (HitReactAnimSequence)
	{
		StopSprinting();
		GetCharacterMovement()->StopMovementImmediately();
		ResetCombo();
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->StopAllMontages(0.1f);
			AnimInstance->PlaySlotAnimationAsDynamicMontage(HitReactAnimSequence, FName("DefaultSlot"), 0.1f, 0.1f, 1.0f, 1);
		}
		bIsHitReacting = true;
		float AnimLength = HitReactAnimSequence->GetPlayLength();
		GetWorldTimerManager().ClearTimer(HitReactResetTimer);
		GetWorldTimerManager().SetTimer(HitReactResetTimer, this, &Ablackmyth_wukongCharacter::ResetHitReactState, AnimLength, false);
	}
	return ActualDamage;
}
void Ablackmyth_wukongCharacter::ResetHitReactState() { bIsHitReacting = false; }
void Ablackmyth_wukongCharacter::Die()
{
	if (bIsDead) return;
	bIsDead = true;
	StopSprinting();
	if (APlayerController* PC = Cast<APlayerController>(GetController())) DisableInput(PC);
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance) AnimInstance->StopAllMontages(0.2f);
	ResetCombo();

	GetWorldTimerManager().ClearTimer(DodgeResetTimer);
	GetWorldTimerManager().ClearTimer(DodgeCooldownTimer);
	GetWorldTimerManager().ClearTimer(ComboResetTimer);
	GetWorldTimerManager().ClearTimer(SkillCooldownTimer);
	GetWorldTimerManager().ClearTimer(HitReactResetTimer);
	DestroyClones();

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	if (DeathAnimSequence && AnimInstance)
	{
		float AnimLength = DeathAnimSequence->GetPlayLength();
		AnimInstance->PlaySlotAnimationAsDynamicMontage(DeathAnimSequence, FName("DefaultSlot"), 0.25f, 0.25f, 1.0f, 1);
		float RagdollDelay = FMath::Max(0.0f, AnimLength - 0.25f);
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

void Ablackmyth_wukongCharacter::TogglePause(const FInputActionValue& Value)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && PauseMenuWidgetClass)
	{
		if (UGameplayStatics::IsGamePaused(GetWorld())) {
			UGameplayStatics::SetGamePaused(GetWorld(), false);
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
			if (PauseMenuInstance) PauseMenuInstance->RemoveFromParent();
		}
		else {
			UGameplayStatics::SetGamePaused(GetWorld(), true);
			PauseMenuInstance = CreateWidget<UUserWidget>(GetWorld(), PauseMenuWidgetClass);
			if (PauseMenuInstance) PauseMenuInstance->AddToViewport();
			PC->bShowMouseCursor = true;
			PC->SetInputMode(FInputModeGameAndUI());
		}
	}
}

void Ablackmyth_wukongCharacter::Sprint() { bIsSprinting = true; GetCharacterMovement()->MaxWalkSpeed = SprintSpeed; }
void Ablackmyth_wukongCharacter::StopSprinting() { bIsSprinting = false; GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; }
void Ablackmyth_wukongCharacter::Tick(float DeltaTime) { Super::Tick(DeltaTime); if (bIsDead || bIsHitReacting || !IdleAnimSequence) return; double CurrentTime = GetWorld()->GetTimeSeconds(); bool bIsMoving = GetVelocity().SizeSquared() > 10.0f; if (bIsMoving || GetCharacterMovement()->IsFalling()) { LastInputTime = CurrentTime; return; } if ((CurrentTime - LastInputTime) > IdleWaitTime) { UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); if (AnimInstance && !AnimInstance->IsAnyMontagePlaying()) { CurrentIdleMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(IdleAnimSequence, FName("DefaultSlot"), 0.25f, 0.25f, 1.0f, 1); } } }
void Ablackmyth_wukongCharacter::ResetIdleTimer() { if (GetWorld()) LastInputTime = GetWorld()->GetTimeSeconds(); UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); if (AnimInstance && CurrentIdleMontage) { if (AnimInstance->Montage_IsPlaying(CurrentIdleMontage)) { AnimInstance->Montage_Stop(0.25f, CurrentIdleMontage); } CurrentIdleMontage = nullptr; } }
void Ablackmyth_wukongCharacter::GainExperience(float Amount) { if (bIsDead) return; CurrentXP += Amount; CheckLevelUp(); }
void Ablackmyth_wukongCharacter::CheckLevelUp() { while (CurrentXP >= MaxXP) { CurrentXP -= MaxXP; CharacterLevel++; MaxHealth += 20.0f; BaseAttackPower += 5.0f; MaxXP = MaxXP * 1.5f; CurrentHealth = MaxHealth; OnLevelUp(); } }
float Ablackmyth_wukongCharacter::GetTotalAttackPower() const { return BaseAttackPower; }
void Ablackmyth_wukongCharacter::ResumeGameFromUI()
{
	// 直接复用 TogglePause 的逻辑来取消暂停
	TogglePause(FInputActionValue());
}