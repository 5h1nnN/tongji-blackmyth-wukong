// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies.h"
#include "SparrowProjectile.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h" // 引入 BoxComponent 头文件
#include "Kismet/GameplayStatics.h"  // 引入玩法统计库（用于造成伤害）
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnemyAIController.h"
#include "BrainComponent.h"
#include "Kismet/KismetMathLibrary.h" // 用于计算朝向旋转

// Sets default values
AEnemies::AEnemies()
{
	// 开启每一帧执行 Tick
	PrimaryActorTick.bCanEverTick = true;

	// 初始化数值
	MaxHealth = 100.f;
	Health = MaxHealth;

	// 初始化右手
    WeaponCollisionR = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollisionR"));
    WeaponCollisionR->SetupAttachment(GetMesh(), FName("FX_Trail_R_02")); // 绑定右手
    WeaponCollisionR->SetBoxExtent(FVector(40.f, 40.f, 60.f));
    WeaponCollisionR->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponCollisionR->SetCollisionResponseToAllChannels(ECR_Ignore);
    WeaponCollisionR->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // 初始化左手
    WeaponCollisionL = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollisionL"));
    WeaponCollisionL->SetupAttachment(GetMesh(), FName("FX_Trail_L_02")); // 绑定左手
    WeaponCollisionL->SetBoxExtent(FVector(40.f, 40.f, 60.f));
    WeaponCollisionL->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponCollisionL->SetCollisionResponseToAllChannels(ECR_Ignore);
    WeaponCollisionL->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // 初始化血条组件
    HealthBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComp"));
    HealthBarWidgetComp->SetupAttachment(GetRootComponent()); // 挂在根部，稍后在蓝图调整位置

    // 设置默认属性
    HealthBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen); // Screen模式会让血条永远面向摄像机
    HealthBarWidgetComp->SetDrawSize(FVector2D(100.f, 10.f)); // 默认大小

    RangedSocketName = TEXT("Muzzle_01");

    // 创建组件
    ImmobilizeIconWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ImmobilizeIcon"));
    ImmobilizeIconWidget->SetupAttachment(GetRootComponent());

    // 设置默认属性
    ImmobilizeIconWidget->SetWidgetSpace(EWidgetSpace::Screen); // Screen模式：始终面向屏幕，不管敌人怎么转
    ImmobilizeIconWidget->SetDrawAtDesiredSize(true);           // 自动调整大小
    ImmobilizeIconWidget->SetVisibility(false);                 // 默认隐藏

    // 设置位置
    ImmobilizeIconWidget->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
}

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

// 处理受击伤害
float AEnemies::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f; // 死了就不再扣血

    // 友军伤害过滤 (防止远程攻击误伤队友)
    if (EventInstigator)
    {
        // 获取造成伤害的控制器的 Pawn (凶手)
        APawn* AttackerPawn = EventInstigator->GetPawn();

        // 如果凶手存在，且凶手也是 AEnemies 类 (或者是其子类)
        if (AttackerPawn && AttackerPawn->IsA(AEnemies::StaticClass()))
        {
            // 如果是自己打自己(比如箭矢刚生成就撞到自己)，或者队友打我
            // 直接忽略伤害
            return 0.f;
        }
    }

    // 1. 扣血
    float DamageApplied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    Health = FMath::Clamp(Health - DamageApplied, 0.f, MaxHealth);
    // 更新 UI
    UpdateHealthUI();

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
        // 只有当 "不处于硬直状态" 时，才触发受击反应
        if (!bIsStunned)
        {
            // 标记进入硬直
            bIsStunned = true;

            // 没死 -> 播放受击动画 (打断当前攻击)
            if (HitMontage)
            {
                PlayAnimMontage(HitMontage);

            }

            // 获取 AI 控制器
            AAIController* AIC = Cast<AAIController>(GetController());
            if (IsAttackerBehind(DamageCauser) && TurnAttackMontage)
            {
                // A. 背后受击 -> 转身反击

                // 1. 强制转向攻击者
                RotateToFaceActor(DamageCauser);

                // 2. 播放转身攻击蒙太奇
                StopAnimMontage(); // 打断当前动作
                PlayAnimMontage(TurnAttackMontage);

                // 为了防止 AI 在播动画时乱跑，可以先 StopMovement
                if (AIC) AIC->StopMovement();

            }
            else {
                // B. 正面受击 -> 普通硬直
                if (AIC)
                {
                    // 立刻停止移动
                    AIC->StopMovement();

                    // 暂停行为树逻辑 (防止它这时候决定攻击你)
                    if (AIC->GetBrainComponent())
                    {
                        AIC->GetBrainComponent()->StopLogic("HitReaction");
                    }
                }


                // 设置定时器：在 StunDuration 秒后，执行 RecoverFromStun 函数
                // 如果再次受击，SetTimer 会自动重置时间（重置硬直）
                GetWorldTimerManager().SetTimer(
                    StunTimerHandle,
                    this,
                    &AEnemies::RecoverFromStun,
                    StunDuration,
                    false
                );
            }
        }
        else {
            // 如果已经在硬直中，只扣血，不打断，不重置定时器
            // if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("Hit ignored due to Stun protection"));
        }
    }


    if (Health <= 0.f)
    {
        // 打印调试
        // if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("death"));
        HandleDeath();
    }
    else
    {
        // 打印调试
        // if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("hit"));

        if (HitMontage)
        {
            PlayAnimMontage(HitMontage);
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
    SpawnLoot();
    // 若干秒后销毁尸体
    SetLifeSpan(2.0f);
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

    // 2. 排除队友：如果受害者也是 AEnemies 类，直接无视
    if (OtherActor->IsA(AEnemies::StaticClass()))
    {
        return; // 是队友，什么都不做，直接返回
    }

    // 如果是 Character (比如玩家)
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

        // 打印调试
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

    bIsStunned = false;

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


bool AEnemies::IsAttackerBehind(AActor* Attacker)
{
    if (!Attacker) return false;

    // 1. 获取从敌人指向攻击者的向量
    FVector ToAttacker = Attacker->GetActorLocation() - GetActorLocation();
    ToAttacker.Z = 0.f; // 忽略高度差
    ToAttacker.Normalize();

    // 2. 获取敌人的正前方向量
    FVector Forward = GetActorForwardVector();

    // 3. 计算点乘 (Dot Product)
    // 结果为 1.0 代表正前方，-1.0 代表正后方，0 代表侧面
    float DotResult = FVector::DotProduct(Forward, ToAttacker);

    // 4. 如果小于 -0.5 (约 135度到225度的扇形区域)，视为背后
    return DotResult < -0.5f;
}

void AEnemies::RotateToFaceActor(AActor* TargetActor)
{
    if (!TargetActor) return;

    // 计算朝向目标的旋转
    FVector Start = GetActorLocation();
    FVector Target = TargetActor->GetActorLocation();
    FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(Start, Target);

    // 只保留 Y轴旋转 (Yaw)，防止敌人歪倒
    FRotator TargetRot(0.f, LookAtRot.Yaw, 0.f);

    // 瞬间转向
    SetActorRotation(TargetRot);
}


void AEnemies::FireRangedAttack()
{
    // 0. 安全检查
    if (!GetWorld() || !GetMesh()) return;
    if (bIsDead) return;

    // 1. 检查有没有设置箭的蓝图
    if (!ProjectileClass)
    {
        // if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error: ProjectileClass is NULL in Enemies BP!"));
        return;
    }

    // 2. 计算发射位置和方向
    FVector SocketLoc;
    FRotator SocketRot;

    // 尝试从插槽获取
    if (GetMesh()->DoesSocketExist(RangedSocketName))
    {
        SocketLoc = GetMesh()->GetSocketLocation(RangedSocketName);

        // 瞄准逻辑
        AAIController* AIC = Cast<AAIController>(GetController());
        if (AIC && AIC->GetFocusActor())
        {
            FVector TargetLoc = AIC->GetFocusActor()->GetActorLocation();
            // 计算从发射点到目标的朝向
            SocketRot = UKismetMathLibrary::FindLookAtRotation(SocketLoc, TargetLoc);
        }
        else
        {
            // 如果没有锁定目标，就沿着插槽朝向发射
            SocketRot = GetMesh()->GetSocketRotation(RangedSocketName);
        }
    }
    else
    {
        // 如果没有插槽，从胸前发射
        SocketLoc = GetActorLocation() + GetActorForwardVector() * 50.f + FVector(0, 0, 50.f);
        SocketRot = GetActorRotation();
    }

    // 3. 配置生成参数
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this; // 确保箭矢知道是敌人射的，OnHit里才能获取 Instigator
    SpawnParams.Instigator = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; // 强制生成

    // 4. 生成箭矢
    GetWorld()->SpawnActor<ASparrowProjectile>(
        ProjectileClass,
        SocketLoc,
        SocketRot,
        SpawnParams
    );
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


void AEnemies::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


void AEnemies::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemies::OnImmobilized_Implementation(float Duration)
{
    UE_LOG(LogTemp, Warning, TEXT(" %s frozen, duration: %f"), *GetName(), Duration);
    // 1. 停止 AI 逻辑 (行为树)
    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        if (UBrainComponent* Brain = AICon->GetBrainComponent())
        {
            Brain->StopLogic("Immobilize");
        }
        AICon->StopMovement();
    }

    // 2. 冻结位移组件 (防止物理或者动量继续移动)
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();

    // 仅冻结 Mesh动画
    GetMesh()->bPauseAnims = true;

    BP_OnImmobilizeVisuals(true);

    // 显示 UI
    if (ImmobilizeIconWidget)
    {
        ImmobilizeIconWidget->SetVisibility(true);
    }

    // 3. 设置定时器自动解除
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_Immobilize, this, &AEnemies::HandleImmobilizeTimeout, Duration, false);
}

void AEnemies::HandleImmobilizeTimeout()
{
    // 调用接口的解除函数
    Execute_OnUnImmobilized(this);
}

void AEnemies::OnUnImmobilized_Implementation()
{
    // 清除定时器 (如果是被攻击提前打断，需要手动清除)
    GetWorld()->GetTimerManager().ClearTimer(TimerHandle_Immobilize);

    // 1. 恢复 AI
    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        if (UBrainComponent* Brain = AICon->GetBrainComponent())
        {
            Brain->RestartLogic();
        }
    }

    // 2. 恢复移动模式
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);

    // 3. 恢复动画
    GetMesh()->bPauseAnims = false;

    // 4. 关闭视觉特效
    BP_OnImmobilizeVisuals(false);
    // 隐藏 UI
    if (ImmobilizeIconWidget)
    {
        ImmobilizeIconWidget->SetVisibility(false);
    }
}
void AEnemies::SpawnLoot()
{
    // 1. 如果列表为空，直接返回
    if (LootDropList.Num() == 0) return;

    // 2. 随机选取一个物品索引
    int32 RandomIndex = FMath::RandRange(0, LootDropList.Num() - 1);
    TSubclassOf<ABaseFood> ItemClassToSpawn = LootDropList[RandomIndex];

    if (ItemClassToSpawn)
    {
        // 3. 设定生成位置 (在敌人位置稍微向上抬高)
        FVector SpawnLocation = GetActorLocation() + FVector(0.f, 0.f, 50.f);
        FRotator SpawnRotation = GetActorRotation(); // 或者使用 FRotator::ZeroRotator

        // 4. 强制生成，即使碰撞重叠
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        SpawnParams.Owner = this;

        // 5. 生成 Actor
        GetWorld()->SpawnActor<ABaseFood>(ItemClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);

    }
}