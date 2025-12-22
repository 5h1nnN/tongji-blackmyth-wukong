// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "FeyCharacter.generated.h"

// 前置声明
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class BLACKMYTH_WUKONG_API AFeyCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	// 构造函数
	AFeyCharacter();

protected:
	// 游戏开始或生成时调用
	virtual void BeginPlay() override;

public:
	// 每帧调用
	virtual void Tick(float DeltaTime) override;

	// 绑定输入功能
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// --- 摄像机组件 ---
protected:
	/** 摄像机吊杆，用于将摄像机定位在角色身后 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** 跟随摄像机 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	// --- 移动逻辑函数 ---
protected:
	/** 向前/向后移动 */
	void MoveForward(float Value);

	/** 向左/向右移动 */
	void MoveRight(float Value);

	// --- 生命值系统 ---
public:
	/** 最大生命值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float MaxHealth;

	/** 当前生命值 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	float CurrentHealth;

	/**
	 * 受到伤害的处理函数 (重写自 AActor)
	 * @param DamageAmount 伤害数值
	 * @return 实际受到的伤害
	 */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/** 获取当前生命值百分比 (用于UI) */
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetHealthPercent() const;
};