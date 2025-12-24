#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

class AFeyCharacter;

UCLASS()
class BLACKMYTH_WUKONG_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseCharacter();
	// --- 战斗参数配置 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float CurrentHealth;

	// 在蓝图中指定要变身的目标类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	TSubclassOf<ABaseCharacter> TargetCharacterClass;

	// 变身特效 (Niagara 或 粒子)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	class UNiagaraSystem* TransformationVFX;

	// 变身核心函数
	UFUNCTION(BlueprintCallable, Category = "Transformation")
	void TransformCharacter();

	/** 冷却总时长 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	float TransformCooldownDuration = 30.0f;

	/** 是否处于冷却中 */
	UPROPERTY(BlueprintReadOnly, Category = "Transformation")
	bool bIsCooldown = false;

	/** 启动冷却计时 */
	void StartTransformCooldown();

	/** 冷却结束的回调 */
	void OnCooldownFinished();

	/** 获取剩余冷却百分比 (给UI用, 0.0 - 1.0) */
	UFUNCTION(BlueprintCallable, Category = "Transformation")
	float GetCooldownPercent() const;

	FTimerHandle CooldownTimerHandle;

protected:

	// 增强输入相关
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* TransformAction;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};