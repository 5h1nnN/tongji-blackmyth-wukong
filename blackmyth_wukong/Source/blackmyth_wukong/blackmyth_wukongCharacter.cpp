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
#include "ImmobilizableInterface.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include"Public/MyGameInstance.h"
#include "Components/BrushComponent.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

Ablackmyth_wukongCharacter::Ablackmyth_wukongCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	WalkSpeed = 500.0f;
	SprintSpeed = 900.0f;
	bIsSprinting = false;
	bIsDead = false;

	DodgeCooldownTime = 0.5f;
	DodgePlayRate = 1.3f;
	DodgeStrength = 770.0f;

	CloneCount = 3;
	CloneSpawnRadius = 300.0f;
	CloneLifeSpan = 15.0f;
	CloneSkillCooldown = 30.0f;
	bAutoSpawnNavMesh = true;

	SkillCooldownTime = 5.0f;
	CharacterLevel = 1;
	BaseAttackPower = 10.0f;
	IdleWaitTime = 10.0f;

	bIsImmobilizeOnCooldown = false;
}

void Ablackmyth_wukongCharacter::BeginPlay()
{
	Super::BeginPlay();
	UMyGameInstance* MyGI = Cast<UMyGameInstance>(GetGameInstance());
	if (MyGI)
	{
		// 只有当 SavedHealth > 0 时才读取（防止第一次进游戏变成 -1 血）
		if (MyGI->SavedMaxHealth > 0.0f)
		{
			this->MaxHealth = MyGI->SavedMaxHealth;
			this->CurrentXP = MyGI->SavedXP;
			this->CharacterLevel = MyGI->SavedLevel;
		}
	}
	// 确保状态重置
	CurrentHealth = MaxHealth;
	bIsDead = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->StopMovementImmediately();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		// 1. 允许角色接收输入
		EnableInput(PlayerController);

		// 2. 将输入模式从 UIOnly (可能残留的状态) 切回 GameOnly
		FInputModeGameOnly GameInputMode;
		PlayerController->SetInputMode(GameInputMode);

		// 3. 隐藏鼠标光标
		PlayerController->bShowMouseCursor = false;
		// 把输入模式切回游戏（允许键盘移动）
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}

		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->ViewPitchMin = -40.0f;
			PlayerController->PlayerCameraManager->ViewPitchMax = 5.0f;
		}
	}

	ResetIdleTimer();
	SpawnDynamicNavMesh();

	// 创建并显示常驻 HUD (进度条等)
	if (HUDWidgetClass)
	{
		if (!HUDInstance)
		{
			HUDInstance = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
		}

		if (HUDInstance)
		{
			HUDInstance->AddToViewport();
		}
	}
}

void Ablackmyth_wukongCharacter::SpawnDynamicNavMesh()
{
	if (!bAutoSpawnNavMesh) return;

	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> FoundVolumes;
	UGameplayStatics::GetAllActorsOfClass(World, ANavMeshBoundsVolume::StaticClass(), FoundVolumes);
	if (FoundVolumes.Num() > 0) return;

	FVector SpawnLoc = GetActorLocation();
	SpawnLoc.Z = 0;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ANavMeshBoundsVolume* NavVolume = World->SpawnActor<ANavMeshBoundsVolume>(ANavMeshBoundsVolume::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);

	if (NavVolume)
	{
		FVector NewScale = NavMeshExtent / 100.0f;
		NavVolume->SetActorScale3D(NewScale);

		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			NavSys->OnNavigationBoundsUpdated(NavVolume);
			NavSys->Build();
		}
	}
}

void Ablackmyth_wukongCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 调用父类 Setup，父类会自动绑定 PauseAction 和 TransformAction
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &Ablackmyth_wukongCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &Ablackmyth_wukongCharacter::Look);

		if (LightAttackAction)
			EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformLightAttack);
		if (HeavyAttackAction)
			EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformHeavyAttack);
		if (DodgeAction)
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformDodge);
		if (SprintAction) {
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::Sprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &Ablackmyth_wukongCharacter::StopSprinting);
		}

		if (SpecialSkillAction)
			EnhancedInputComponent->BindAction(SpecialSkillAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformSpecialSkill);

		if (CloneAction)
			EnhancedInputComponent->BindAction(CloneAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::PerformCloneSkill);

		if (ImmobilizeAction)
			EnhancedInputComponent->BindAction(ImmobilizeAction, ETriggerEvent::Started, this, &Ablackmyth_wukongCharacter::Immobilize);

		// 注意：PauseAction 的绑定已移至 BaseCharacter::SetupPlayerInputComponent
	}
}

void Ablackmyth_wukongCharacter::AddHealth(float Amount)
{
	CurrentHealth += Amount;

	// 可以在这里加个简单的限制，比如不超过100
	if (CurrentHealth > MaxHealth)
	{
		CurrentHealth = MaxHealth;
	}

}

void Ablackmyth_wukongCharacter::AddXP(float Amount)
{
	// 1. 增加经验
	CurrentXP += Amount;
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
		LevelUp();
	}
}


void Ablackmyth_wukongCharacter::PerformCloneSkill(const FInputActionValue& Value)
{
	if (bIsCloneSkillCooldown || bIsDead || bIsHitReacting || bIsDodging || GetCharacterMovement()->IsFalling()) return;

	GetWorldTimerManager().ClearTimer(CloneLifeTimer);

	ResetIdleTimer();
	StopSprinting();
	GetCharacterMovement()->StopMovementImmediately();

	DestroyClones();

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (CloneSummonMontage && AnimInstance)
	{
		AnimInstance->Montage_Play(CloneSummonMontage);
	}

	UClass* SpawnClass = CloneClass ? CloneClass.Get() : GetClass();
	if (!SpawnClass) return;

	FVector CenterLoc = GetActorLocation();
	FRotator SpawnRot = GetActorRotation();
	float AngleStep = 360.0f / (CloneCount > 0 ? CloneCount : 1);

	for (int32 i = 0; i < CloneCount; i++)
	{
		float CurrentAngle = i * AngleStep;
		float Rad = FMath::DegreesToRadians(CurrentAngle);

		FVector Offset(FMath::Cos(Rad) * CloneSpawnRadius, FMath::Sin(Rad) * CloneSpawnRadius, 0.0f);
		FVector PotentialLoc = CenterLoc + Offset;

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
		SpawnParams.Owner = this;

		ACharacter* NewClone = GetWorld()->SpawnActor<ACharacter>(SpawnClass, FinalLoc, SpawnRot, SpawnParams);
		if (NewClone)
		{
			NewClone->SpawnDefaultController();
			NewClone->Tags.Add(FName("Clone"));

			if (CloneSpawnFX)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), CloneSpawnFX, FinalLoc);
			}

			ActiveClones.Add(NewClone);
		}
	}

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
		return FMath::Clamp(Remaining / CloneSkillCooldown, 0.0f, 1.0f);
	}
	return 0.0f;
}

float Ablackmyth_wukongCharacter::GetCloneRechargePercent() const
{
	return 1.0f - GetCloneCooldownFraction();
}

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

	if ((HeavyAttackMontage && AnimInstance->Montage_IsPlaying(HeavyAttackMontage)) ||
		(CurrentSkillMontage && AnimInstance->Montage_IsPlaying(CurrentSkillMontage)) ||
		(CloneSummonMontage && AnimInstance->Montage_IsPlaying(CloneSummonMontage))) return;

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
	if (!AnimInstance || (CurrentSkillMontage && AnimInstance->Montage_IsPlaying(CurrentSkillMontage))) return;

	if (!AnimInstance->Montage_IsPlaying(HeavyAttackMontage))
	{
		AnimInstance->StopAllMontages(0.2f);
		AnimInstance->Montage_Play(HeavyAttackMontage);
		FTimerHandle HitCheckTimer;
		GetWorldTimerManager().SetTimer(HitCheckTimer, [this]() { CheckAttackHit(150.0f, 1.5f); }, 0.4f, false);
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
		GetWorldTimerManager().SetTimer(HitCheckTimer, [this]() { CheckAttackHit(400.0f, 3.3f); }, 0.3f, false);
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
		return FMath::Clamp(Remaining / SkillCooldownTime, 0.0f, 1.0f);
	}
	return 0.0f;
}

void Ablackmyth_wukongCharacter::CheckAttackHit(float CurrentRange, float DamageMultiplier)
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

	if (GetOwner()) ActorsToIgnore.Add(GetOwner());
	for (ACharacter* Clone : ActiveClones) if (Clone) ActorsToIgnore.Add(Clone);

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
				// 计算最终伤害：基础攻击力 * 倍率
				float FinalDamage = GetTotalAttackPower() * DamageMultiplier;
				UGameplayStatics::ApplyDamage(HitActor, FinalDamage, GetController(), this, UDamageType::StaticClass());
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
		GetWorldTimerManager().SetTimer(HitReactResetTimer, this, &Ablackmyth_wukongCharacter::ResetHitReactState, HitReactAnimSequence->GetPlayLength(), false);
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

	GetWorldTimerManager().ClearAllTimersForObject(this);
	DestroyClones();

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	if (DeathAnimSequence && AnimInstance)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(DeathAnimSequence, FName("DefaultSlot"), 0.25f, 0.25f, 1.0f, 1);
		FTimerHandle TimerHandle_Ragdoll;
		GetWorldTimerManager().SetTimer(TimerHandle_Ragdoll, [this]()
			{
				GetMesh()->SetSimulatePhysics(true);
				GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
				GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				GetMesh()->bPauseAnims = true;
			}, DeathAnimSequence->GetPlayLength() - 0.25f, false);
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FTimerHandle TimerHandle_ShowUI;
		GetWorldTimerManager().SetTimer(TimerHandle_ShowUI, [this, PC]()
			{
				if (GameOverWidgetClass)
				{
					UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), GameOverWidgetClass);
					if (Widget) { Widget->AddToViewport(); PC->bShowMouseCursor = true; PC->SetInputMode(FInputModeUIOnly()); }
				}
			}, 2.0f, false);
	}
}

void Ablackmyth_wukongCharacter::Sprint() { bIsSprinting = true; GetCharacterMovement()->MaxWalkSpeed = SprintSpeed; }
void Ablackmyth_wukongCharacter::StopSprinting() { bIsSprinting = false; GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; }

void Ablackmyth_wukongCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bIsDead || bIsHitReacting || !IdleAnimSequence) return;
	double CurrentTime = GetWorld()->GetTimeSeconds();
	if (GetVelocity().SizeSquared() > 10.0f || GetCharacterMovement()->IsFalling()) { LastInputTime = CurrentTime; return; }
	if ((CurrentTime - LastInputTime) > IdleWaitTime) {
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && !AnimInstance->IsAnyMontagePlaying()) {
			CurrentIdleMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(IdleAnimSequence, FName("DefaultSlot"), 0.25f, 0.25f, 1.0f, 1);
		}
	}
}

void Ablackmyth_wukongCharacter::ResetIdleTimer()
{
	if (GetWorld()) LastInputTime = GetWorld()->GetTimeSeconds();
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CurrentIdleMontage) {
		if (AnimInstance->Montage_IsPlaying(CurrentIdleMontage)) AnimInstance->Montage_Stop(0.25f, CurrentIdleMontage);
		CurrentIdleMontage = nullptr;
	}
}

void Ablackmyth_wukongCharacter::GainExperience(float Amount) { if (!bIsDead) { CurrentXP += Amount; CheckLevelUp(); } }
float Ablackmyth_wukongCharacter::GetTotalAttackPower() const { return BaseAttackPower; }

void Ablackmyth_wukongCharacter::Immobilize(const FInputActionValue& Value)
{
	if (bIsDead || bIsImmobilizeOnCooldown) return;

	CastImmobilizeSkill();
	bIsImmobilizeOnCooldown = true;
	GetWorldTimerManager().SetTimer(TimerHandle_ImmobilizeCooldown, this, &Ablackmyth_wukongCharacter::ResetImmobilizeCooldown, ImmobilizeCooldownTime, false);
}

void Ablackmyth_wukongCharacter::ResetImmobilizeCooldown()
{
	bIsImmobilizeOnCooldown = false;
	GetWorldTimerManager().ClearTimer(TimerHandle_ImmobilizeCooldown);
}

void Ablackmyth_wukongCharacter::CastImmobilizeSkill()
{
	FVector Start = GetActorLocation();
	FVector End = Start + (GetActorForwardVector() * ImmobilizeRange);
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	FHitResult HitResult;

	bool bHit = UKismetSystemLibrary::SphereTraceSingle(this, Start, End, 50.0f, UEngineTypes::ConvertToTraceType(ECC_Pawn), false, ActorsToIgnore, EDrawDebugTrace::None, HitResult, true);

	if (bHit && HitResult.GetActor() && HitResult.GetActor()->Implements<UImmobilizableInterface>())
	{
		IImmobilizableInterface::Execute_OnImmobilized(HitResult.GetActor(), ImmobilizeDuration);
	}
}

float Ablackmyth_wukongCharacter::GetImmobilizeCooldownPercent() const
{
	if (!bIsImmobilizeOnCooldown) return 1.0f;

	if (GetWorldTimerManager().IsTimerActive(TimerHandle_ImmobilizeCooldown))
	{
		return 1.0f - (GetWorldTimerManager().GetTimerRemaining(TimerHandle_ImmobilizeCooldown) / ImmobilizeCooldownTime);
	}

	return 1.0f;
}