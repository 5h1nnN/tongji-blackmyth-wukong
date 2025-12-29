#pragma once

#include "CoreMinimal.h"
#include "Enemies.h" // 继承自基类
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
	// 重写受击函数，用于检测阶段转换
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;


// Boss 基础属性
protected:
	// 是否处于二阶段
	bool bIsPhaseTwo = false;

	// 进入二阶段的血量阈值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss | Stats")
	float PhaseTwoThreshold = 0.5f;

	// 金箍棒专用碰撞盒
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UBoxComponent* WeaponCollisionStaff;

	// Boss 进入二阶段前无敌动画
	UPROPERTY(EditDefaultsOnly, Category = "Boss | Skills")
	UAnimMontage* Montage_PhaseTransition;

	// 召唤分身
	UFUNCTION(BlueprintCallable, Category = "Boss | Skills")
	void SummonClones(int32 NumClones);

	// 分身使用的类
	UPROPERTY(EditDefaultsOnly, Category = "Boss | Skills")
	TSubclassOf<AEnemies> MinionClass;

public:
	// 处理阶段转换
	void EnterPhaseTwo();

	// 重写父类的开碰撞函数
	virtual void EnableWeaponCollision(bool bEnableLeft, bool bEnableRight) override;

	// 重写父类的关碰撞函数
	virtual void DisableWeaponCollision() override;

	// 开启/关闭金箍棒碰撞
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetStaffCollision(bool bActive);

	// 重写基类的重叠检测，因为金箍棒是新的组件
	UFUNCTION()
	void OnStaffOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);


// 无敌机制
protected:
	// 是否处于无敌状态
	bool bIsInvincible = false;

	// 用于自动关闭无敌的定时器
	FTimerHandle InvincibilityTimerHandle;

	// 关闭无敌状态的函数 (给定时器调用)
	void DisableInvincibility();

	// 无敌时的特效
	UPROPERTY(EditDefaultsOnly, Category = "Boss | FX")
	UParticleSystem* InvincibilityFX;

	// 保存生成的特效组件引用，以便无敌结束时销毁它
	UPROPERTY()
	UParticleSystemComponent* ActiveInvincibilityFXComp;


// 分身机制
public:
    // 标记当前对象是否为分身
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss | Stats")
    bool bIsClone = false;

protected:
	// 延迟生成分身的定时器
	FTimerHandle SpawnTimerHandle;

	// 辅助函数：专门用来在定时器结束后生成分身
	void SpawnPhaseTwoMinions();

	// 存放当前存活的分身列表
	// UPROPERTY() 能够防止指针对应的对象被意外垃圾回收，且方便调试
	UPROPERTY(VisibleAnywhere, Category = "Boss | Minions")
	TArray<AEnemies*> ActiveMinions;

	// 重写死亡逻辑
	virtual void HandleDeath() override;

	// 清理所有分身的辅助函数
	void KillAllMinions();


// Boss 专有血量显示
protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UBossHealthBar> BossHUDClass;

	// 运行时实例也用子类指针保存																																																				
	UPROPERTY()
	UBossHealthBar* BossHUDInstance;

	virtual void UpdateHealthUI() override;

protected:
	// 在编辑器中分配"胜利/游戏结束"的UI蓝图类
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> VictoryWidgetClass;
};