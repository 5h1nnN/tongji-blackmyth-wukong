// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemies.h" // 继承自你之前的基类
#include "BossHealthBar.h"
#include "EnemyBoss.generated.h"

class UBoxComponent;
class UParticleSystem;
class UParticleSystemComponent;

/**
 * Paragon Wukong Boss 类
 */
UCLASS()
class BLACKMYTH_WUKONG_API AEnemyBoss : public AEnemies
{
	GENERATED_BODY()

public:
	AEnemyBoss();

protected:
	virtual void BeginPlay() override;

public:
	// 重写受击函数，用于检测阶段转换（Phase Transition）
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

protected:
	// --- Boss 专属数值 ---
	// 是否处于二阶段（狂暴）
	bool bIsPhaseTwo = false;

	// 进入二阶段的血量阈值 (0.0 ~ 1.0, 比如 0.5 代表 50%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss | Stats")
	float PhaseTwoThreshold = 0.5f;

	// --- Boss 专属组件 ---
	// 金箍棒的专用碰撞盒 (比普通拳头大得多)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UBoxComponent* WeaponCollisionStaff;

	// --- Boss 技能蒙太奇 ---
	// 1. 重击 (Slam)
	UPROPERTY(EditDefaultsOnly, Category = "Boss | Skills")
	UAnimMontage* Montage_StaffSlam;

	// 2. 旋风斩 (Spin)
	UPROPERTY(EditDefaultsOnly, Category = "Boss | Skills")
	UAnimMontage* Montage_SpinAttack;

	// 3. 进入二阶段的吼叫/变身动画
	UPROPERTY(EditDefaultsOnly, Category = "Boss | Skills")
	UAnimMontage* Montage_PhaseTransition;

	// --- 技能逻辑 ---
	// 召唤分身 (Wukong 特色)
	UFUNCTION(BlueprintCallable, Category = "Boss | Skills")
	void SummonClones(int32 NumClones);

	// 分身使用的类 (通常是普通 Enemy 的弱化版)
	UPROPERTY(EditDefaultsOnly, Category = "Boss | Skills")
	TSubclassOf<AEnemies> MinionClass;

public:
	// 处理阶段转换
	void EnterPhaseTwo();

	// [新增] 重写父类的开碰撞函数
	virtual void EnableWeaponCollision(bool bEnableLeft, bool bEnableRight) override;

	// [新增] 重写父类的关碰撞函数
	virtual void DisableWeaponCollision() override;

	// 开启/关闭金箍棒碰撞 (给动画通知调用)
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetStaffCollision(bool bActive);

	// 重写基类的重叠检测，因为金箍棒是新的组件
	UFUNCTION()
	void OnStaffOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);


protected:
	// --- 无敌机制 ---

	// 是否处于无敌状态
	bool bIsInvincible = false;

	// 用于自动关闭无敌的定时器
	FTimerHandle InvincibilityTimerHandle;

	// 关闭无敌状态的函数 (给定时器调用)
	void DisableInvincibility();

	// [可选] 无敌时的特效 (比如一个金钟罩或护盾)
	UPROPERTY(EditDefaultsOnly, Category = "Boss | FX")
	UParticleSystem* InvincibilityFX;

	// 保存生成的特效组件引用，以便无敌结束时销毁它
	UPROPERTY()
	UParticleSystemComponent* ActiveInvincibilityFXComp;

public:
    // 标记当前对象是否为分身
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss | Stats")
    bool bIsClone = false;

protected:
	// [新增] 延迟生成分身的定时器
	FTimerHandle SpawnTimerHandle;

	// [新增] 辅助函数：专门用来在定时器结束后生成分身
	void SpawnPhaseTwoMinions();

protected:
	// [新增] 存放当前存活的分身列表
	// UPROPERTY() 能够防止指针对应的对象被意外垃圾回收，且方便调试
	UPROPERTY(VisibleAnywhere, Category = "Boss | Minions")
	TArray<AEnemies*> ActiveMinions;

	// [新增] 重写死亡逻辑
	virtual void HandleDeath() override;

	// [新增] 清理所有分身的辅助函数
	void KillAllMinions();

protected:
	// [修改] 这里使用具体的子类 TSubclassOf<UBossHealthBar>
	// 这样在编辑器里，你只能选 Boss 专用的 UI 蓝图，防呆
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UBossHealthBar> BossHUDClass;

	// [修改] 运行时实例也用子类指针保存																																																				
	UPROPERTY()
	UBossHealthBar* BossHUDInstance;

	virtual void UpdateHealthUI() override;

protected:
	// 新增：在编辑器中分配"胜利/游戏结束"的UI蓝图类
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> VictoryWidgetClass;
};