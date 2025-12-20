// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "blackmyth_wukongCharacter.generated.h"

// 前向声明
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UAnimMontage;
class UAnimSequence;
class UUserWidget;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class Ablackmyth_wukongCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	// --- [新增] 奔跑输入 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

	// --- 战斗输入动作 ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LightAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* HeavyAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DodgeAction;

	// --- 动画资源 ---

	/** 轻攻击连招蒙太奇数组 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> LightAttackMontages;

	/** 重攻击蒙太奇 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HeavyAttackMontage;

	/** 闪避动画序列 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
	UAnimSequence* DodgeAnimSequence;

	/** 死亡动画序列 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
	UAnimSequence* DeathAnimSequence;

public:
	Ablackmyth_wukongCharacter();

	// 必须重写 Tick 来处理 Idle 逻辑
	virtual void Tick(float DeltaTime) override;

	// --- 战斗参数配置 ---

	/** 最大血量 (升级会自动增加) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth;

	/** 当前血量 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float CurrentHealth;

	/** 闪避冷却时间 (秒) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DodgeCooldownTime;

	/** 闪避播放速率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DodgePlayRate;

	/** 闪避冲刺力度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DodgeStrength;

	/** 死亡时显示的 UI 类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> GameOverWidgetClass;

	// --- [新增] 移动参数 ---

	/** 正常行走速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed;

	/** 奔跑速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed;

	// =================================================================
	// [RPG 升级系统 变量]
	// =================================================================

	/** 当前等级 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RPG System")
	int32 CharacterLevel;

	/** 当前经验值 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RPG System")
	float CurrentXP;

	/** 升级所需经验值 (自动计算) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RPG System")
	float MaxXP;

	/** 基础攻击力 (升级会自动增加) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseAttackPower;

	// =================================================================
	// [RPG 升级系统 函数]
	// =================================================================

	/** 获取经验值 */
	UFUNCTION(BlueprintCallable, Category = "RPG System")
	void GainExperience(float Amount);

	/** 获取当前最终攻击力 (可用于 ApplyDamage) */
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetTotalAttackPower() const;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Die();

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay();

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	// --- [新增] 奔跑处理 ---
	void Sprint();
	void StopSprinting();

	// --- 战斗处理 ---
	void PerformLightAttack(const FInputActionValue& Value);
	void PerformHeavyAttack(const FInputActionValue& Value);
	void PerformDodge(const FInputActionValue& Value);

	void ResetCombo();

	/** 重置闪避的"动作"状态 (恢复移动/攻击/摩擦力) */
	void ResetDodgeState();

	/** 重置闪避的"冷却"状态 (恢复再次闪避的能力) */
	void ResetDodgeCooldown();

	// --- [RPG 保护函数] ---

	/** 检查是否可以升级 (自动增加属性) */
	void CheckLevelUp();

	/** 升级事件 (蓝图可实现，用于播放特效/声音) */
	UFUNCTION(BlueprintImplementableEvent, Category = "RPG System")
	void OnLevelUp();

	// =================================================================
	// [Idle 闲置系统]
	// =================================================================

	/** 多少秒不操作后播放闲置动画 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Idle")
	float IdleWaitTime;

	/** 闲置时播放的动画序列 (Anim Sequence) - [修改点] */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Idle")
	UAnimSequence* IdleAnimSequence;

	/** 更新输入时间 (如果有任何操作，调用此函数) */
	void ResetIdleTimer();

public:
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	int32 ComboIndex;
	bool bIsAttacking;
	FTimerHandle ComboResetTimer;

	// --- 闪避状态管理 ---
	bool bIsDodging;
	bool bDodgeOnCooldown;
	FTimerHandle DodgeResetTimer;
	FTimerHandle DodgeCooldownTimer;

	// --- 死亡状态管理 ---
	bool bIsDead;

	// --- 奔跑状态 ---
	bool bIsSprinting;

	// --- Idle 状态管理 ---
	double LastInputTime;

	/** [新增] 运行时生成的临时蒙太奇引用，用于停止动画 */
	UPROPERTY()
	UAnimMontage* CurrentIdleMontage;
};