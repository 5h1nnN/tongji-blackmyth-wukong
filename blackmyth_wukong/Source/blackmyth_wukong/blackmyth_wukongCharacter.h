// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "BaseCharacter.h"
#include "blackmyth_wukongCharacter.generated.h"

// 前向声明
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UAnimMontage;
class UAnimSequence;
class UUserWidget; // [新增] 必须声明，否则识别不了 UI 类
struct FInputActionValue;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SpecialSkillAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LightAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* HeavyAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DodgeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* PauseAction;

	// --- 动画资源 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
	TArray<UAnimMontage*> LightAttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HeavyAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
	UAnimSequence* SpecialSkillAnimSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
	UAnimSequence* HitReactAnimSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
	UAnimSequence* DodgeAnimSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
	UAnimSequence* DeathAnimSequence;

public:
	Ablackmyth_wukongCharacter();

	virtual void Tick(float DeltaTime) override;

	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DodgeCooldownTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DodgePlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DodgeStrength;

	// --- 攻击判定参数 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitDetection")
	float AttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitDetection")
	float SkillAttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitDetection")
	float AttackRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitDetection")
	bool bShowHitDebug;

	// --- 技能参数 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Skill")
	float SkillCooldownTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Skill")
	bool bIsSkillOnCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> GameOverWidgetClass;


	/** 暂停菜单的蓝图类 (WBP_PauseMenu) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> PauseMenuWidgetClass;

	/** 保存当前的菜单实例 */
	UPROPERTY()
	UUserWidget* PauseMenuInstance;

	// --- 移动参数 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed;

	// --- RPG 系统 ---
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

	UFUNCTION(BlueprintPure, Category = "Combat|Skill")
	float GetSkillCooldownFraction() const;

	UFUNCTION(BlueprintCallable, Category = "Combat|HitDetection")
	void CheckAttackHit(float CurrentRange);

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Die();

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay();

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Sprint();
	void StopSprinting();

	void PerformLightAttack(const FInputActionValue& Value);
	void PerformHeavyAttack(const FInputActionValue& Value);
	void PerformDodge(const FInputActionValue& Value);
	void PerformSpecialSkill(const FInputActionValue& Value);
	void ResetSkillCooldown();
	void ResetCombo();
	void ResetDodgeState();
	void ResetDodgeCooldown();
	void ResetHitReactState();
	void CheckLevelUp();

	void TogglePause(const FInputActionValue& Value);

	UFUNCTION(BlueprintImplementableEvent, Category = "RPG System")
	void OnLevelUp();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Idle")
	float IdleWaitTime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Idle")
	UAnimSequence* IdleAnimSequence;

	void ResetIdleTimer();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResumeGameFromUI();

public:
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	int32 ComboIndex;
	bool bIsAttacking;
	FTimerHandle ComboResetTimer;
	bool bIsDodging;
	bool bDodgeOnCooldown;
	FTimerHandle DodgeResetTimer;
	FTimerHandle DodgeCooldownTimer;
	FTimerHandle SkillCooldownTimer;
	bool bIsDead;
	bool bIsSprinting;
	bool bIsHitReacting;
	FTimerHandle HitReactResetTimer;
	double LastInputTime;

	UPROPERTY()
	UAnimMontage* CurrentIdleMontage;

	UPROPERTY()
	UAnimMontage* CurrentSkillMontage;
};