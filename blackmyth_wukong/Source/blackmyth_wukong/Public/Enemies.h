// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/WidgetComponent.h"
#include "EnemyHealthBar.h"
#include "BaseFood.h"
#include "ImmobilizableInterface.h"
#include "Enemies.generated.h"

class ABaseFood;
class UBoxComponent;
class ASparrowProjectile;

UCLASS()
class BLACKMYTH_WUKONG_API AEnemies : public ACharacter, public IImmobilizableInterface
{
    GENERATED_BODY()

public:
    // 构造函数
    AEnemies();

    // 重写系统的受击函数，处理扣血逻辑
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

    // 定身接口实现
    virtual void OnImmobilized_Implementation(float Duration);
    virtual void OnUnImmobilized_Implementation();

protected:
    // 开始游戏时调用
    virtual void BeginPlay() override;

public:
    // 当前血量：允许在编辑器修改，并在蓝图中读写
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat | Stats")
    float Health = 100.f;

    // 最大血量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat | Stats")
    float MaxHealth = 100.f;

    // 敌人名称（用于显示在血条上）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat | Stats")
    FText EnemyName;

protected:
    // 1. 武器碰撞盒组件
    // 右手武器碰撞盒
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    UBoxComponent* WeaponCollisionR;

    // 左手武器碰撞盒
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    UBoxComponent* WeaponCollisionL;

    // 2. 基础攻击力
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float BaseDamage = 20.f;

    // 3. 记录单次挥刀已击中的敌人（防止一刀多判）
    UPROPERTY()
    TArray<AActor*> HitActors;


    // 用于计时自动解除定身
    FTimerHandle TimerHandle_Immobilize;

    // 处理定身结束的回调
    void HandleImmobilizeTimeout();

    // 蓝图事件，用来播放材质特效
    UFUNCTION(BlueprintImplementableEvent)
    void BP_OnImmobilizeVisuals(bool bStart);

public:
    // 4. 开启武器碰撞（给动画通知调用）
    // bEnableLeft: 是否开启左手, bEnableRight: 是否开启右手
    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void EnableWeaponCollision(bool bEnableLeft, bool bEnableRight);

    // 5. 关闭武器碰撞（给动画通知调用）
    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void DisableWeaponCollision();

    // 6. 重叠判定函数（必须加 UFUNCTION 宏）
    UFUNCTION()
    void OnWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

protected:
    // 受击蒙太奇 (在蓝图中赋值)
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    UAnimMontage* HitMontage;

    // 死亡蒙太奇 (在蓝图中赋值)
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    UAnimMontage* DeathMontage;

    // 是否已死亡 (防止鞭尸)
    bool bIsDead = false;

    // 处理死亡的函数
    virtual void HandleDeath();

public:
    // 获取是否死亡 (给 AI 控制器用)
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool IsDead() const { return bIsDead; }

protected:
    // 硬直控制
    // 
    // 硬直时间 (可以在蓝图调整，默认 1.5秒)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float StunDuration = 1.5f;

    // 定时器句柄 (用于管理倒计时)
    FTimerHandle StunTimerHandle;

    // 恢复行动的函数
    void RecoverFromStun();


protected:
    // 转身攻击蒙太奇 (在蓝图中赋值)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    UAnimMontage* TurnAttackMontage;

    // 判断攻击者是否在身后
    bool IsAttackerBehind(AActor* Attacker);

    // 强制转向攻击者的辅助函数
    void RotateToFaceActor(AActor* TargetActor);


protected:
    // 箭矢蓝图类 (在编辑器里选 BP_SparrowArrow)
    UPROPERTY(EditDefaultsOnly, Category = "Combat | Ranged")
    TSubclassOf<ASparrowProjectile> ProjectileClass;

    // 发射插槽名称 (Paragon 资源通常叫 "Muzzle_01" 或 "WeaponSocket")
    UPROPERTY(EditDefaultsOnly, Category = "Combat | Ranged")
    FName RangedSocketName;

public:
    // 发射函数 (供动画通知调用)
    UFUNCTION(BlueprintCallable, Category = "Combat | Ranged")
    void FireRangedAttack();


protected:
    // UI 组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* HealthBarWidgetComp;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* ImmobilizeIconWidget;

    // 更新血条的辅助函数
    virtual void UpdateHealthUI();

public:
    // 每一帧调用
    virtual void Tick(float DeltaTime) override;

    // 绑定输入（敌人通常不需要，可以保留为空）
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
    // [新增] 标记是否处于硬直状态
    bool bIsStunned = false;

    // [新增] 获取是否处于硬直 (如果需要给子类判断用)
    bool IsStunned() const {
        return bIsStunned;

    }
protected:

    // [新增] 掉落物列表：允许在蓝图中配置掉落哪些 BaseFood 的子类
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    TArray<TSubclassOf<ABaseFood>> LootDropList;

    // [新增] 执行掉落的函数
    virtual void SpawnLoot();
};

