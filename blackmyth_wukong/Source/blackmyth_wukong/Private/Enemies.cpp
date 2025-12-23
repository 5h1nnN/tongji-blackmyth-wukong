// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies.h"
#include "Components/BoxComponent.h" // 引入 BoxComponent 头文件
#include "Kismet/GameplayStatics.h"  // 引入玩法统计库（用于造成伤害）
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnemyAIController.h"
#include "BrainComponent.h"

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

    // 初始化血条组件
    HealthBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComp"));
    HealthBarWidgetComp->SetupAttachment(GetRootComponent()); // 挂在根部，稍后在蓝图调整位置

    // 设置默认属性
    HealthBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen); // Screen模式会让血条永远面向摄像机
    HealthBarWidgetComp->SetDrawSize(FVector2D(100.f, 10.f)); // 默认大小
}

// Called when the game starts or when spawned	
void AEnemies::BeginPlay()
{
    Super::BeginPlay();

    // 确保刚出生时是满血
    Health = MaxHealth;
    UpdateHealthUI(); 

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
    // 更新 UI
    UpdateHealthUI();
    // UE_LOG(LogTemp, Warning, TEXT("敌人剩余血量: %f"), Health);

    // 2. 判断死亡
    if (Health <= 0.f)
    {
        // 死了就把血条隐藏
        if (HealthBarWidgetComp) HealthBarWidgetComp->SetVisibility(false);
        HandleDeath();
        // 如果死了，清除硬直定时器
        GetWorldTimerManager().ClearTimer(StunTimerHandle); 

    }
    else
    {
        // 3. 没死 -> 播放受击动画 (打断当前攻击)
        if (HitMontage)
        {
            PlayAnimMontage(HitMontage);

        }
        
        // A. 获取 AI 控制器
        AAIController* AIC = Cast<AAIController>(GetController());
        if (AIC)
        {
            // B. 物理打断：立刻停止移动
            AIC->StopMovement();
            
            // C. 精神打断：暂停行为树逻辑 (防止它这时候决定攻击你)
            if (AIC->GetBrainComponent())
            {
                AIC->GetBrainComponent()->StopLogic("HitReaction");
            }
        }

        
        // E. 设置定时器：在 StunDuration 秒后，执行 RecoverFromStun 函数
        // 如果再次受击，SetTimer 会自动重置时间（重置硬直）
        GetWorldTimerManager().SetTimer(
            StunTimerHandle, 
            this, 
            &AEnemies::RecoverFromStun, 
            StunDuration, 
            false
        );
    }


    if (Health <= 0.f)
    {
        // 打印调试
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("death"));
        HandleDeath();
    }
    else
    {
        // 打印调试
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("hit"));

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
    SetLifeSpan(3.0f);
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

        // // 6. 打印调试信息
        // if (GEngine)
        // {
        //     FString Msg = FString::Printf(TEXT("target: %s, damage: %f"), *OtherActor->GetName(), BaseDamage);
        //     GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, Msg);
        // }
    }
}


void AEnemies::RecoverFromStun()
{
    if (bIsDead) return; // 如果硬直期间死了，就不恢复了

    // 获取 AI 控制器
    AAIController* AIC = Cast<AAIController>(GetController());
    if (AIC && AIC->GetBrainComponent())
    {
        // 重启行为树逻辑
        AIC->GetBrainComponent()->RestartLogic();
        
        // 打印调试
        // if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("硬直结束，AI 恢复行动"));
    }
}

void AEnemies::UpdateHealthUI()
{
    if (HealthBarWidgetComp)
    {
        // 获取真正的 Widget 实例，并转换为我们的 C++ 类型
        UEnemyHealthBar* HealthBar = Cast<UEnemyHealthBar>(HealthBarWidgetComp->GetUserWidgetObject());
        if (HealthBar)
        {
            float Percent = Health / MaxHealth;
            HealthBar->UpdateHealthPercent(Percent);
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