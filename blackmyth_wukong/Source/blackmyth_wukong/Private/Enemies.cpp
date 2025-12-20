// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies.h"

// Sets default values
AEnemies::AEnemies()
{
	// 开启每一帧执行 Tick
	PrimaryActorTick.bCanEverTick = true;

	// 初始化数值
	MaxHealth = 100.f;
	Health = MaxHealth;
}

// Called when the game starts or when spawned
void AEnemies::BeginPlay()
{
	Super::BeginPlay();

	// 确保开始游戏时是满血
	Health = MaxHealth;
}

// 核心逻辑：处理受击伤害
float AEnemies::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 1. 调用父类基础逻辑并获取实际伤害值
	float DamageToApply = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 2. 如果已经死亡，则不再处理伤害
	if (Health <= 0.f) return 0.f;

	// 3. 扣除生命值并限制在 0 和 最大血量 之间
	Health = FMath::Clamp(Health - DamageToApply, 0.f, MaxHealth);

	// 4. 打印调试信息（在左上角显示，方便你测试）
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("敌人受到伤害: %f, 当前血量: %f"), DamageToApply, Health));

	// 5. 判断死亡
	if (Health <= 0.f)
	{
		// 这里可以调用死亡函数（比如播放死亡动画、销毁物体等）
		UE_LOG(LogTemp, Warning, TEXT("敌人已阵亡！"));
	}
	else
	{
		// 这里可以调用受击反馈函数（比如播放受击蒙太奇）
	}

	return DamageToApply;
}

// Called every frame
void AEnemies::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AEnemies::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}