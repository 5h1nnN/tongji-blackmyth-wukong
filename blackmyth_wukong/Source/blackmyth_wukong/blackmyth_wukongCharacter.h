// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h" // 确保这是你的父类头文件
#include "Logging/LogMacros.h"
#include "blackmyth_wukongCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UAnimMontage;
class UAnimSequence;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class Ablackmyth_wukongCharacter : public ABaseCharacter
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

	// [新增] 分身术输入动作
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CloneAction;

	// --- 基础攻击/动作 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LightAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* HeavyAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DodgeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* PauseAction;

	// [新增] 特殊技能动作
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SpecialSkillAction;

public:
	Ablackmyth_wukongCharacter();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResumeGameFromUI();

	void PerformLightAttack(const FInputActionValue& Value);
	void PerformHeavyAttack(const FInputActionValue& Value);
	void PerformDodge(const FInputActionValue& Value);
	void PerformSpecialSkill(const FInputActionValue& Value);

	// =================================================================
	// [新增] 自动导航网格生成参数
	// =================================================================

	/** 是否在开始时自动生成 NavMeshBoundsVolume */
	UPROPERTY(EditAnywhere, Category = "AI|Navigation")
	bool bAutoSpawnNavMesh = true;

	/** 自动生成的导航网格覆盖半径 (长宽高的一半) */
	UPROPERTY(EditAnywhere, Category = "AI|Navigation")
	FVector NavMeshExtent = FVector(5000.0f, 5000.0f, 1000.0f);

	// =================================================================
	// [新增] 分身术系统参数
	// =================================================================

	/** 分身使用的蓝图类 (建议创建 BP_Wukong_Clone) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Clone")
	TSubclassOf<ACharacter> CloneClass;

	/** 分身数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Clone")
	int32 CloneCount = 3;

	/** 生成半径 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Clone")
	float CloneSpawnRadius = 300.0f;

	/** 分身存活时间 (秒) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Clone")
	float CloneLifeSpan = 15.0f;

	/** 技能冷却时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Clone")
	float CloneSkillCooldown = 30.0f;

	/** 分身冷却状态 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Clone")
	bool bIsCloneSkillCooldown = false;

	/** 召唤动作 Montage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Clone")
	UAnimMontage* CloneSummonMontage;

	/** 生成特效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Clone")
	class UParticleSystem* CloneSpawnFX;

	// =================================================================
	// 其他现有参数 (保持不变)
	// =================================================================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
	TArray<UAnimMontage*> LightAttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
	UAnimMontage* HeavyAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
	UAnimSequence* DodgeAnimSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
	UAnimSequence* SpecialSkillAnimSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
	UAnimSequence* HitReactAnimSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
	UAnimSequence* DeathAnimSequence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DodgeCooldownTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DodgePlayRate;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DodgeStrength;

	// 技能参数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Skill")
	float SkillCooldownTime;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Skill")
	bool bIsSkillOnCooldown;

	// UI 类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> PauseMenuWidgetClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> GameOverWidgetClass;
	UPROPERTY()
	UUserWidget* PauseMenuInstance;

	// 移动参数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed;

	// RPG 系统
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RPG System")
	int32 CharacterLevel;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RPG System")
	float CurrentXP;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RPG System")
	float MaxXP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseAttackPower;

	// --- 函数 ---
	UFUNCTION(BlueprintCallable, Category = "RPG System")
	void GainExperience(float Amount);
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetTotalAttackPower() const;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Die();

protected:
	virtual void BeginPlay();
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;

	/** 处理移动 */
	void Move(const FInputActionValue& Value);
	/** 处理视角 */
	void Look(const FInputActionValue& Value);

	// 动作函数
	void Sprint();
	void StopSprinting();
	void TogglePause(const FInputActionValue& Value);

	// [新增] 分身术实现函数
	void PerformCloneSkill(const FInputActionValue& Value);
	void DestroyClones();
	void ResetCloneSkillCooldown();

	// [新增] 自动生成导航网格
	void SpawnDynamicNavMesh();

	// 辅助函数
	void ResetCombo();
	void ResetDodgeState();
	void ResetDodgeCooldown();
	void ResetSkillCooldown();
	void ResetHitReactState();
	void CheckAttackHit(float CurrentRange);
	void CheckLevelUp();
	UFUNCTION(BlueprintImplementableEvent, Category = "RPG System")
	void OnLevelUp();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Idle")
	float IdleWaitTime;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Idle")
	UAnimSequence* IdleAnimSequence;
	void ResetIdleTimer();

public:
	/** 获取分身冷却进度 (0.0 - 1.0) */
	UFUNCTION(BlueprintPure, Category = "Combat|Clone")
	float GetCloneCooldownFraction() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Skill")
	float GetSkillCooldownFraction() const;

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	// 分身管理
	UPROPERTY()
	TArray<ACharacter*> ActiveClones;
	FTimerHandle CloneLifeTimer;
	FTimerHandle CloneCooldownTimer;

	// 状态标记
	int32 ComboIndex;
	bool bIsAttacking;
	bool bIsDodging;
	bool bDodgeOnCooldown;
	bool bIsSprinting;
	bool bIsDead;
	bool bIsHitReacting;

	FTimerHandle ComboResetTimer;
	FTimerHandle DodgeResetTimer;
	FTimerHandle DodgeCooldownTimer;
	FTimerHandle SkillCooldownTimer;
	FTimerHandle HitReactResetTimer;
	double LastInputTime;

	UPROPERTY()
	UAnimMontage* CurrentIdleMontage;
	UPROPERTY()
	UAnimMontage* CurrentSkillMontage;
};