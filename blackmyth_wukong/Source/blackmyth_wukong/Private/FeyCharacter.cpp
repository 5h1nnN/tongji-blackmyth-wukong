#include "FeyCharacter.h"
#include "BaseProjectile.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

AFeyCharacter::AFeyCharacter()
{
	// 设置此角色每帧调用 Tick()
	PrimaryActorTick.bCanEverTick = true;

	// --- 1. 初始化摄像机组件 ---
	// 创建弹簧臂
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // 摄像机距离角色的距离
	CameraBoom->bUsePawnControlRotation = true; // 随控制器旋转（鼠标控制视角）

	// 创建跟随摄像机
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // 附着在吊杆末端
	FollowCamera->bUsePawnControlRotation = false; // 摄像机不直接旋转，而是跟随吊杆

	// --- 2. 配置角色移动特性 ---
	// 也就是不要让角色的胶囊体随控制器旋转（那是摄像机做的事），只让角色面向移动方向
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 配置角色移动组件
	GetCharacterMovement()->bOrientRotationToMovement = true; // 角色朝向输入的方向移动
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // 转身速率
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;

	// --- 3. 初始化属性 ---
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
}

void AFeyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 关键：当角色生成时，如果是玩家控制，添加输入映射上下文
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			// 即使变身，也要确保这个角色的映射表被激活
			Subsystem->AddMappingContext(FeyMappingContext, 0);
		}
	}
	if (AutoTransformDuration > 0.0f)
	{
		// 参数说明：句柄，对象，函数地址，延迟时间，是否循环
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
	// 时间到了，执行变身
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("Time Up! Auto Reverting..."));

	TransformCharacter();
}

void AFeyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 注意：不调用 Super，或者确保 Super 里没有冲突逻辑
	// 我们在这里完全重写增强输入绑定

	if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 1. 绑定移动 (Vector2D)
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFeyCharacter::Move);
		}

		// 2. 绑定视角 (Vector2D)
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

		// 4. 绑定变身 (调用父类 BaseCharacter 的 TransformCharacter)
		if (TransformAction)
		{
			EIC->BindAction(TransformAction, ETriggerEvent::Started, this, &ABaseCharacter::TransformCharacter);
		}
		if (AttackAction)
		{
			EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AFeyCharacter::Attack);
		}
	}
}

void AFeyCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// 获取控制旋转的偏航角 (Yaw)
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// 获取前方向量
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		// 获取右方向量
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// 添加移动
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AFeyCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// 添加控制器水平/垂直输入
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
// 实现 Attack
void AFeyCharacter::Attack()
{
	if (ProjectileClass)
    {
        FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 50.f;
        FRotator SpawnRotation = GetControlRotation();
		SpawnRotation.Pitch = 0.0f; // 强制水平
		SpawnRotation.Roll = 0.0f;  // 强制水平

        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.Instigator = this;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        GetWorld()->SpawnActor<ABaseProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, Params);
		// 播放攻击动画
		if (AttackMontage)
		{
			PlayAnimMontage(AttackMontage);
		}
	}
}