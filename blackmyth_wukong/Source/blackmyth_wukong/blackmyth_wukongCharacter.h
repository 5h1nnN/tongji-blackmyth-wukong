// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h" // 确保包含父类头文件
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
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	//UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CloneAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LightAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* HeavyAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DodgeAction;

	// [已删除] PauseAction (已移至 BaseCharacter)
	// UInputAction* PauseAction; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SpecialSkillAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ImmobilizeAction;

public:
	Ablackmyth_wukongCharacter();

	// [已删除] ResumeGameFromUI (已移至 BaseCharacter，且无需重写)
	// void ResumeGameFromUI();

	void PerformLightAttack(const FInputActionValue& Value);
	void PerformHeavyAttack(const FInputActionValue& Value);
	void PerformDodge(const FInputActionValue& Value);
	void PerformSpecialSkill(const FInputActionValue& Value);

	// --- 导航与分身系统参数 ---
	UPROPERTY(EditAnywhere, Category = "AI|Navigation")
	bool bAutoSpawnNavMesh = true;

	UPROPERTY(EditAnywhere, Category = "AI|Navigation")
	FVector NavMeshExtent = FVector(5000.0f, 5000.0f, 1000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Clone")
	TSubclassOf<ACharacter> CloneClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Clone")
	int32 CloneCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Clone")
	float CloneSpawnRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Clone")
	float CloneLifeSpan = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Clone")
	float CloneSkillCooldown = 30.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Clone")
	bool bIsCloneSkillCooldown = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Clone")
	UAnimMontage* CloneSummonMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Clone")
	class UParticleSystem* CloneSpawnFX;

	// --- 动画与战斗参数 ---
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Skill")
	float SkillCooldownTime;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Skill")
	bool bIsSkillOnCooldown;

	// --- UI 系统 ---
	// [已删除] PauseMenuWidgetClass (已移至 BaseCharacter)
	// TSubclassOf<class UUserWidget> PauseMenuWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> GameOverWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> HUDWidgetClass;

	// [已删除] PauseMenuInstance (已移至 BaseCharacter)
	// UUserWidget* PauseMenuInstance;

	// [新增] 保存 HUD 实例
	UPROPERTY()
	UUserWidget* HUDInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RPG System")
	int32 CharacterLevel;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RPG System")
	float CurrentXP;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RPG System")
	float MaxXP=100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseAttackPower;

	UFUNCTION(BlueprintCallable, Category = "RPG System")
	void GainExperience(float Amount);
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetTotalAttackPower() const;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Die();
	UFUNCTION(BlueprintCallable, Category = "RPG System")
	void AddXP(float Amount);
	UFUNCTION(BlueprintImplementableEvent, Category = "RPG System")
	void LevelUp();
	UFUNCTION(BlueprintCallable, Category = "RPG System")
	void AddHealth(float Amount);
protected:
	virtual void BeginPlay();
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Sprint();
	void StopSprinting();

	// [已删除] TogglePause (已移至 BaseCharacter)
	// void TogglePause(const FInputActionValue& Value);

	// 分身术功能
	void PerformCloneSkill(const FInputActionValue& Value);
	void DestroyClones();
	void ResetCloneSkillCooldown();

	// 自动生成导航网格
	void SpawnDynamicNavMesh();

	// 定身术功能
	void Immobilize(const FInputActionValue& Value);
	void CastImmobilizeSkill();
	void ResetImmobilizeCooldown();

	// 辅助逻辑
	void ResetCombo();
	void ResetDodgeState();
	void ResetDodgeCooldown();
	void ResetSkillCooldown();
	void ResetHitReactState();
	void CheckAttackHit(float CurrentRange);
	UFUNCTION(BlueprintCallable, Category = "RPG System")
	void CheckLevelUp();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Idle")
	float IdleWaitTime;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Idle")
	UAnimSequence* IdleAnimSequence;
	void ResetIdleTimer();

public:
	UFUNCTION(BlueprintPure, Category = "Combat|Clone")
	float GetCloneCooldownFraction() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Clone")
	float GetCloneRechargePercent() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Skill")
	float GetSkillCooldownFraction() const;

	UFUNCTION(BlueprintPure, Category = "Skills|UI")
	float GetImmobilizeCooldownPercent() const;

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	UPROPERTY()
	TArray<ACharacter*> ActiveClones;
	FTimerHandle CloneLifeTimer;
	FTimerHandle CloneCooldownTimer;

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

protected:
	UPROPERTY(EditAnywhere, Category = "Skills")
	float ImmobilizeRange = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Skills")
	float ImmobilizeDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
	float ImmobilizeCooldownTime = 10.0f;

	FTimerHandle TimerHandle_ImmobilizeCooldown;
	bool bIsImmobilizeOnCooldown;
};