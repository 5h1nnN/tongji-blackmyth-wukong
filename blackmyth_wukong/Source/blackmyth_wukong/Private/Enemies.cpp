// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies.h"
#include "Components/BoxComponent.h" // 引入 BoxComponent 头文件
#include "Kismet/GameplayStatics.h"  // 引入玩法统计库（用于造成伤害）
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


// Sets default values
AEnemies::AEnemies()
{
	// 开启每一帧执行 Tick
	PrimaryActorTick.bCanEverTick = true;

	// 初始化数值
	MaxHealth = 100.f;
	Health = MaxHealth;

	// --- 1. 初始化右手 (原有的) ---
    WeaponCollisionR = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollisionR"));
    WeaponCollisionR->SetupAttachment(GetMesh(), FName("FX_Trail_R_02")); // 绑定右手
    WeaponCollisionR->SetBoxExtent(FVector(30.f, 30.f, 30.f));
    WeaponCollisionR->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponCollisionR->SetCollisionResponseToAllChannels(ECR_Ignore);
    WeaponCollisionR->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // --- 2. 初始化左手 (新增的) ---
    WeaponCollisionL = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollisionL"));
    WeaponCollisionL->SetupAttachment(GetMesh(), FName("FX_Trail_L_02")); // 绑定左手
    WeaponCollisionL->SetBoxExtent(FVector(30.f, 30.f, 30.f));
    WeaponCollisionL->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponCollisionL->SetCollisionResponseToAllChannels(ECR_Ignore);
    WeaponCollisionL->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

// Called when the game starts or when spawned	
void AEnemies::BeginPlay()
{
    Super::BeginPlay();

    // 确保开始游戏时是满血
    Health = MaxHealth;

    // 绑定事件
    if (WeaponCollisionR)
    {
        WeaponCollisionR->OnComponentBeginOverlap.AddDynamic(this, &AEnemies::OnWeaponOverlap);
    }

    if (WeaponCollisionL)
    {
        WeaponCollisionL->OnComponentBeginOverlap.AddDynamic(this, &AEnemies::OnWeaponOverlap);
    }
}

// 核心逻辑：处理受击伤害
float AEnemies::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f; // 死了就不再扣血

    // 1. 扣血
    float DamageApplied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    Health = FMath::Clamp(Health - DamageApplied, 0.f, MaxHealth);

    UE_LOG(LogTemp, Warning, TEXT("敌人剩余血量: %f"), Health);

    // 2. 判断死亡
    if (Health <= 0.f)
    {
        HandleDeath();
    }
    else
    {
        // 3. 没死 -> 播放受击动画 (打断当前攻击)
        if (HitMontage)
        {
            PlayAnimMontage(HitMontage);
        }
    }


    if (Health <= 0.f)
    {
        // 打印调试
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("00death"));
        HandleDeath();
    }
    else
    {
        // 打印调试
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("11hit"));

        if (HitMontage)
        {
            PlayAnimMontage(HitMontage);
        }
        else
        {
            // 关键调试：如果这一行出来了，说明你在蓝图里没选资源！
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ERROR"));
        }
    }

    return DamageApplied;
}

// 实现死亡逻辑
void AEnemies::HandleDeath()
{
    if (bIsDead) return;
    bIsDead = true;

    // 禁用碰撞 (防止挡路)
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);


    // 播放死亡动画
    if (DeathMontage)
    {
        PlayAnimMontage(DeathMontage);
    }

    // 若干秒后销毁尸体
    SetLifeSpan(5.0f);
}



// 开启碰撞（攻击开始）
void AEnemies::EnableWeaponCollision(bool bEnableLeft, bool bEnableRight)
{

    // 每次攻击开始时清空受击列表
    HitActors.Empty();
    
    // 开启检测 (QueryOnly 表示只做查询不处理物理碰撞)
    if (bEnableRight && WeaponCollisionR)
    {
        WeaponCollisionR->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }

    if (bEnableLeft && WeaponCollisionL)
    {
        WeaponCollisionL->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
}

// 关闭碰撞（攻击结束）
void AEnemies::DisableWeaponCollision()
{
    if (WeaponCollisionR)
    {
        WeaponCollisionR->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    
    if (WeaponCollisionL)
    {
        WeaponCollisionL->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

// 核心伤害逻辑
void AEnemies::OnWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 1. 基本检查：必须有对象，且不是自己
    if (!OtherActor || OtherActor == this) return;

    // 2. 阵营检查：如果是 Character (比如玩家)
    // 这里简单用 Cast 判断，实际项目中通常用 Tag 或 Interface 区分敌我
    if (OtherActor->IsA(ACharacter::StaticClass())) 
    {
        // 3. 防重复检查：如果这一刀已经砍过这个人，就不再扣血
        if (HitActors.Contains(OtherActor)) return;

        // 4. 加入已攻击列表
        HitActors.Add(OtherActor);

        // 5. 造成伤害
        UGameplayStatics::ApplyDamage(
            OtherActor,           // 受害者
            BaseDamage,           // 伤害值
            GetController(),      // 凶手控制器
            this,                 // 凶手本体
            UDamageType::StaticClass() // 伤害类型
        );

        // 6. 打印调试信息
        if (GEngine)
        {
            FString Msg = FString::Printf(TEXT("砍中了: %s, 造成伤害: %f"), *OtherActor->GetName(), BaseDamage);
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, Msg);
        }
    }
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