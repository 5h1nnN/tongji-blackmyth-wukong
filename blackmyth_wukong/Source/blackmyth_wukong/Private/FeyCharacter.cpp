#include "FeyCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	// 游戏开始时重置血量
	CurrentHealth = MaxHealth;
}

void AFeyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// 绑定输入 (注意：这里使用的是传统的轴映射，如果是UE5.1+ 建议使用 Enhanced Input)
void AFeyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 绑定移动轴 (需在项目设置 -> 输入 中配置名称为 "MoveForward" 和 "MoveRight")
	PlayerInputComponent->BindAxis("MoveForward", this, &AFeyCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFeyCharacter::MoveRight);

	// 绑定鼠标视角轴 (UE默认轴名称)
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	// 绑定跳跃 (使用 ACharacter 自带的函数)
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
}

void AFeyCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// 找出前方在哪里
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// 获取前方向量
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AFeyCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// 找出右方在哪里
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// 获取右方向量
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

float AFeyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 调用父类逻辑（如果有的话）
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 简单的减血逻辑
	ActualDamage = FMath::Min(CurrentHealth, ActualDamage);
	CurrentHealth -= ActualDamage;

	// 打印调试信息 (屏幕左上角)
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("HP: %f / %f"), CurrentHealth, MaxHealth));
	}

	// 如果血量归零，处理死亡逻辑
	if (CurrentHealth <= 0.0f)
	{
		// TODO: 播放死亡动画或销毁角色
		// Destroy(); 
	}

	return ActualDamage;
}

float AFeyCharacter::GetHealthPercent() const
{
	return CurrentHealth / MaxHealth;
}