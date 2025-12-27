#include "EnemyBoss.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

AEnemyBoss::AEnemyBoss()
{
    // 1. 强化数值
    MaxHealth = 1000.f; // Boss 血量更高
    Health = MaxHealth;
    BaseDamage = 20.f;  // Boss 伤害更高
    EnemyName = FText::FromString("GreatSage"); // UI 显示名字

    // 2. 调整体型 (Boss 通常比普通怪大一点)
    GetCapsuleComponent()->SetCapsuleSize(45.f, 110.f);

    // 3. 调整移动 (Wukong 比较灵活)
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 400.f, 0.f); // 转身更快

    // 4. 初始化金箍棒碰撞盒
    WeaponCollisionStaff = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollisionStaff"));
    // 绑定到右手或特定插槽 (Paragon Wukong 武器插槽通常叫 "weapon_r" 或 "FX_Trail_R_02")
    WeaponCollisionStaff->SetupAttachment(GetMesh(), FName("weapon_r"));
    // 金箍棒很长，设置一个长条形的碰撞盒
    WeaponCollisionStaff->SetBoxExtent(FVector(10.f, 10.f, 120.f));
    // 稍微向下偏移一点，覆盖整根棍子
    WeaponCollisionStaff->SetRelativeLocation(FVector(0.f, 0.f, -40.f));

    // 默认关闭碰撞
    WeaponCollisionStaff->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponCollisionStaff->SetCollisionResponseToAllChannels(ECR_Ignore);
    WeaponCollisionStaff->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AEnemyBoss::BeginPlay()
{
    Super::BeginPlay();

    // 绑定金箍棒的碰撞事件
    if (WeaponCollisionStaff)
    {
        WeaponCollisionStaff->OnComponentBeginOverlap.AddDynamic(this, &AEnemyBoss::OnStaffOverlap);
    }
}

float AEnemyBoss::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // 无敌检查
    if (bIsInvincible)
    {
        // 可以播放一个“叮”的金属音效，提示玩家攻击无效
        // UGameplayStatics::PlaySoundAtLocation(this, BlockSound, GetActorLocation());

        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Blue, TEXT("Boss is Invincible!"));
        return 0.f; // 直接返回，不扣血
    }

    float PendingHealth = Health - DamageAmount;
    if (!bIsClone && !bIsPhaseTwo && (PendingHealth / MaxHealth) <= PhaseTwoThreshold)
    {
        float ActualDamage = DamageAmount;
        Health = FMath::Clamp(PendingHealth, 0.f, MaxHealth);
        UpdateHealthUI();

        // 2. 检查是否死亡，死了就不处理阶段了
        if (bIsDead) return ActualDamage;

        StunDuration = 10.0f;
        EnterPhaseTwo();

        return ActualDamage;
    }

    return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AEnemyBoss::EnterPhaseTwo()
{
    bIsPhaseTwo = true;

    bIsInvincible = true;

    bIsStunned = true;

    // 生成无敌特效 (如果有设置)
    if (InvincibilityFX)
    {
        // SpawnEmitterAttached 会让特效跟着 Boss 移动
        ActiveInvincibilityFXComp = UGameplayStatics::SpawnEmitterAttached(
            InvincibilityFX,
            GetMesh(),
            FName("spine_02"), // 挂在脊柱或中心位置
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget
        );
        if (ActiveInvincibilityFXComp)
        {
            ActiveInvincibilityFXComp->CustomTimeDilation = 0.4f;
        }
    }

    // 1. 播放转阶段动画
    float Duration = 5.0f;

    if (Montage_PhaseTransition)
    {
        // 2. 停止 AI 逻辑 (防止它在定格时移动或攻击)
        AAIController* AIC = Cast<AAIController>(GetController());
        if (AIC)
        {
            // 停止移动
            AIC->StopMovement();
            AIC->ClearFocus(EAIFocusPriority::Gameplay);

            // 停止行为树逻辑 (BrainComponent)
            if (AIC->GetBrainComponent())
            {
                AIC->GetBrainComponent()->StopLogic("PhaseTwoFreeze");
            }
            UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent());
            if (BTComp)
            {
                // StopTree 会发送 Abort 信号给当前正在运行的蓝图 Task
                // 从而触发你刚才写的 "Event Receive Abort AI"
                BTComp->StopTree(EBTStopMode::Safe);
            }
        }

        // 停止当前所有动作
        StopAnimMontage();
        float MontageLen = PlayAnimMontage(Montage_PhaseTransition);
    }


    // 设置定时器自动关闭无敌
    GetWorldTimerManager().SetTimer(
        InvincibilityTimerHandle,
        this,
        &AEnemyBoss::DisableInvincibility,
        Duration,
        false
    );

    GetWorldTimerManager().SetTimer(
        SpawnTimerHandle,
        this,
        &AEnemyBoss::SpawnPhaseTwoMinions,
        Duration,
        false
    );

    // 2. 强化属性
    BaseDamage *= 1.5f; // 伤害提升 50%
    GetCharacterMovement()->MaxWalkSpeed = 700.f; // 移动变快


    //// 4. 打印调试
    //if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BOSS ENRAGED: PHASE 2!"));
}

void AEnemyBoss::SummonClones(int32 NumClones)
{
    if (bIsClone) return;
    
    if (!MinionClass || !GetWorld()) return;

    FVector MyLoc = GetActorLocation();

    // 清理一下数组，移除掉之前可能已经自然死亡或被销毁的分身 (空指针)
    // 这一步不是必须的，但能保持数组干净
    ActiveMinions.RemoveAll([](AEnemies* Ptr) { return Ptr == nullptr || Ptr->IsDead(); });

    for (int32 i = 0; i < NumClones; i++)
    {
        // 计算分身生成位置
        float DirectionMultiplier = (i % 2 == 0) ? 1.0f : -1.0f;
        FVector RightDir = GetActorRightVector();   // 右方向
        FVector SpawnLoc = MyLoc + (RightDir * 400.0f * DirectionMultiplier);
        SpawnLoc.Z = MyLoc.Z; // 保持高度一致

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AEnemies* Clone = GetWorld()->SpawnActor<AEnemies>(MinionClass, SpawnLoc, GetActorRotation(), SpawnParams);

        if (Clone)
        {
            AEnemyBoss* BossClone = Cast<AEnemyBoss>(Clone);
            BossClone->bIsClone = true;

            // 可以削弱分身血量
            BossClone->MaxHealth = MaxHealth * 0.1f;
            BossClone->BaseDamage = BaseDamage * 0.5f;
            BossClone->HealthBarWidgetComp->SetVisibility(false);

            ActiveMinions.Add(Clone);

            // 给分身生成特效
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), nullptr, SpawnLoc); // 这里填入烟雾特效
        }
    }
}


// 重写开启逻辑
void AEnemyBoss::EnableWeaponCollision(bool bEnableLeft, bool bEnableRight)
{
    // 逻辑映射：
    // 只要 Notify 想要开启左手 或 右手 (通常攻击蒙太奇都会勾选其中一个)
    // 我们就开启金箍棒
    if (bEnableLeft || bEnableRight)
    {
        SetStaffCollision(true);
    }
}

// 重写关闭逻辑
void AEnemyBoss::DisableWeaponCollision()
{
    // 调用父类是为了保险 (虽然 Boss 没有左右手碰撞盒，但调用一下无妨)
    Super::DisableWeaponCollision();

    // 关闭金箍棒
    SetStaffCollision(false);
}

void AEnemyBoss::SetStaffCollision(bool bActive)
{
    if (!WeaponCollisionStaff) return;

    if (bActive)
    {
        HitActors.Empty(); // 清空受击列表 (使用父类定义的 HitActors)
        WeaponCollisionStaff->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
    else
    {
        WeaponCollisionStaff->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

// 专门处理长棍的伤害逻辑
void AEnemyBoss::OnStaffOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this) return;
    if (OtherActor->IsA(AEnemies::StaticClass())) return; // 不打队友

    // 复用父类的判定逻辑：是否是角色，是否打过
    if (OtherActor->IsA(ACharacter::StaticClass()) && !HitActors.Contains(OtherActor))
    {
        HitActors.Add(OtherActor);

        // 造成伤害
        UGameplayStatics::ApplyDamage(
            OtherActor,
            BaseDamage, // 此时可能已经是二阶段强化过的伤害
            GetController(),
            this,
            UDamageType::StaticClass()
        );

        // 播放打击音效或特效 (Wukong 金箍棒打击感)
        // UGameplayStatics::SpawnEmitterAtLocation(...);
    }
}

void AEnemyBoss::DisableInvincibility()
{
    bIsInvincible = false;

    bIsStunned = false;
    StunDuration = 0.5f;

    // 销毁无敌特效
    if (ActiveInvincibilityFXComp)
    {
        ActiveInvincibilityFXComp->Deactivate(); // 停止发射粒子
        ActiveInvincibilityFXComp->DestroyComponent(); // 销毁组件
        ActiveInvincibilityFXComp = nullptr;
    }

    // 2. 恢复 AI 逻辑
    AAIController* AIC = Cast<AAIController>(GetController());
    if (AIC && AIC->GetBrainComponent())
    {
        AIC->GetBrainComponent()->RestartLogic();

        UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent());
        if (BTComp)
        {
            // 如果你知道行为树资源，可以用 StartTree
            // 如果之前已经在运行，RestartTree 也是可行的
            BTComp->RestartTree();
        }
    }

    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("Boss Invincibility Ended!"));
}

void AEnemyBoss::SpawnPhaseTwoMinions()
{
    // 确保 Boss 还没死
    if (bIsDead) return;

    // 播放烟雾特效 (移到这里，分身出来时才有烟雾)
    UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), nullptr, GetActorLocation());

    // 生成 2 个分身
    SummonClones(2);
}

void AEnemyBoss::HandleDeath()
{
    // 1. 如果我是本体，就处死所有分身
    if (!bIsClone)
    {
        KillAllMinions();
    }

    // 2. [关键] 必须调用父类的逻辑，执行原本的死亡动画、碰撞关闭等
    Super::HandleDeath();
}

void AEnemyBoss::KillAllMinions()
{
    // 遍历所有记录的分身
    for (AEnemies* Minion : ActiveMinions)
    {
        // 检查指针是否有效，且分身还没死
        if (Minion && !Minion->IsDead())
        {
            // 方法一：直接造成巨额伤害 (推荐)
            // 这样做的好处是会触发分身自己的 TakeDamage -> HandleDeath 流程
            // 分身会播放死亡动画，而不是突然消失
            UGameplayStatics::ApplyDamage(
                Minion,
                99999.f,             // 巨额伤害
                GetController(),     // 凶手是本体的控制器
                this,                // 凶手是本体
                UDamageType::StaticClass()
            );

            // 方法二：如果你想让分身直接消失，不播动画
            // Minion->Destroy();
        }
    }

    // 清空列表
    ActiveMinions.Empty();
}