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

	// --- [新增] 特殊技能输入 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SpecialSkillAction;

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

	/** 特殊技能动画序列 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
	UAnimSequence* SpecialSkillAnimSequence;

	/** 闪避动画序列 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
	UAnimSequence* DodgeAnimSequence;

	/** 死亡动画序列 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
	UAnimSequence* DeathAnimSequence;

public:
	Ablackmyth_wukongCharacter();

	virtual void Tick(float DeltaTime) override;

	// --- 战斗参数配置 ---

	/** 最大血量 */
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

	// --- [新增] 攻击判定参数 ---

	/** 攻击判定距离 (前方多少厘米) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitDetection")
	float AttackRange;

	/** 攻击判定球体半径 (判定宽度) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitDetection")
	float AttackRadius;

	/** 是否显示调试球 (Debug Sphere) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitDetection")
	bool bShowHitDebug;

	// --- 技能参数配置 ---

	/** 技能冷却时间 (秒) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Skill")
	float SkillCooldownTime;

	/** 技能是否处于冷却中 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Skill")
	bool bIsSkillOnCooldown;

	/** 死亡时显示的 UI 类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> GameOverWidgetClass;

	// --- 移动参数 ---

	/** 正常行走速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed;

	/** 奔跑速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed;

	// =================================================================
	// [RPG 升级系统 变量]
	// =================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RPG System")
	int32 CharacterLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RPG System")
	float CurrentXP;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RPG System")
	float MaxXP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseAttackPower;

	// =================================================================
	// [RPG 升级系统 函数]
	// =================================================================

	UFUNCTION(BlueprintCallable, Category = "RPG System")
	void GainExperience(float Amount);

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetTotalAttackPower() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Skill")
	float GetSkillCooldownFraction() const;

	// --- [新增] 蓝图可调用的攻击检测函数 (推荐在 Montage Notify 中调用) ---
	UFUNCTION(BlueprintCallable, Category = "Combat|HitDetection")
	void CheckAttackHit();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Die();

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay();

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void Sprint();
	void StopSprinting();

	// --- 战斗处理 ---
	void PerformLightAttack(const FInputActionValue& Value);
	void PerformHeavyAttack(const FInputActionValue& Value);
	void PerformDodge(const FInputActionValue& Value);

	void PerformSpecialSkill(const FInputActionValue& Value);
	void ResetSkillCooldown();

	void ResetCombo();

	void ResetDodgeState();
	void ResetDodgeCooldown();

	// --- [RPG 保护函数] ---
	void CheckLevelUp();

	UFUNCTION(BlueprintImplementableEvent, Category = "RPG System")
	void OnLevelUp();

	// =================================================================
	// [Idle 闲置系统]
	// =================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Idle")
	float IdleWaitTime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Idle")
	UAnimSequence* IdleAnimSequence;

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

	// --- 技能计时器 ---
	FTimerHandle SkillCooldownTimer;

	// --- 死亡状态管理 ---
	bool bIsDead;

	// --- 奔跑状态 ---
	bool bIsSprinting;

	// --- Idle 状态管理 ---
	double LastInputTime;

	UPROPERTY()
	UAnimMontage* CurrentIdleMontage;

	UPROPERTY()
	UAnimMontage* CurrentSkillMontage;
};