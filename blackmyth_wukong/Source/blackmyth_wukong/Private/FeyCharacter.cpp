#include "FeyCharacter.h"
#include "BaseProjectile.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "TimerManager.h" // 需要包含 TimerManager

AFeyCharacter::AFeyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// --- 1. 初始化摄像机组件 ---
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// --- 2. 配置角色移动特性 ---
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;

	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;

	// [错误修复] 绝对不要在构造函数里调用 GetController 或 AddMappingContext！
	// 此时 Controller 还是空的。逻辑移至 BeginPlay。
}

void AFeyCharacter::BeginPlay()
{
	// 1. 调用父类 (ABaseCharacter) 的 BeginPlay
	// 父类会在这里加载 DefaultMappingContext (包含暂停键、变身键)
	Super::BeginPlay();

	// 2. 加载 Fey 特有的输入映射 (包含移动、跳跃、攻击)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			// Priority 设为 1，确保它的优先级高于默认(0)，防止冲突（如果有的话）
			if (FeyMappingContext)
			{
				Subsystem->AddMappingContext(FeyMappingContext, 1);
			}
		}
	}

	// 3. 自动变身回原形的计时器
	if (AutoTransformDuration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			TransformTimerHandle,
			this,
			&AFeyCharacter::OnAutoTransformTimerTimeout,
			AutoTransformDuration,
			false
		);
	}
}

void AFeyCharacter::OnAutoTransformTimerTimeout()
{
	// 时间到了，执行变身 (调用父类逻辑)
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("Time Up! Auto Reverting..."));
	TransformCharacter();
}

void AFeyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// [关键修复] 必须调用父类，父类会绑定 "暂停 (PauseAction)" 和 "变身 (TransformAction)"
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 1. 绑定移动
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFeyCharacter::Move);
		}

		// 2. 绑定视角
		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFeyCharacter::Look);
		}

		// 3. 绑定跳跃
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}

		// 4. 绑定攻击
		if (AttackAction)
		{
			EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AFeyCharacter::Attack);
		}

		// [注意] 不要在这里绑定 TransformAction，父类 Super::SetupPlayerInputComponent 已经绑定过了
	}
}

void AFeyCharacter::Move(const FInputActionValue& Value)
{
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

void AFeyCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AFeyCharacter::Attack()
{
	if (AttackMontage)
	{
		PlayAnimMontage(AttackMontage);
	}
}

void AFeyCharacter::ExecuteSpawnProjectile()
{
	if (ProjectileClass)
	{
		// 发射位置向前偏移一点，避免和自身碰撞
		FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 100.f;
		FRotator SpawnRotation = GetActorRotation();
		SpawnRotation.Pitch = 0.0f;
		SpawnRotation.Roll = 0.0f;

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<ABaseProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, Params);
	}
}