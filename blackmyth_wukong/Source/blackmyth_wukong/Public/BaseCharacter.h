#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "BaseCharacter.generated.h"

class AFeyCharacter;
class UUserWidget;
class UInputAction;
class UNiagaraSystem;
class UInputMappingContext; // 前置声明

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

	// --- 变身系统 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	TSubclassOf<ABaseCharacter> TargetCharacterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	UNiagaraSystem* TransformationVFX;

	UFUNCTION(BlueprintCallable, Category = "Transformation")
	void TransformCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	float TransformCooldownDuration = 30.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Transformation")
	bool bIsCooldown = false;

	void StartTransformCooldown();
	void OnCooldownFinished();

	UFUNCTION(BlueprintCallable, Category = "Transformation")
	float GetCooldownPercent() const;

	FTimerHandle CooldownTimerHandle;

	// --- UI 与 暂停系统 ---

	/** 暂停菜单的 Widget 类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	/** 保存暂停菜单实例引用 */
	UPROPERTY()
	UUserWidget* PauseMenuInstance;

	/** 提供给 UI 蓝图调用的恢复游戏函数 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResumeGameFromUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void RestartLevel();

	/** [新增] 用于在 UI 中绑定的退出游戏 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void QuitGame();

protected:
	virtual void BeginPlay() override;

	// --- 增强输入相关 (移动至此，供所有子类使用) ---

	/** 默认输入映射上下文 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	/** 变身按键 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* TransformAction;

	/** 暂停按键 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* PauseAction;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** 暂停逻辑处理函数 */
	void TogglePause(const FInputActionValue& Value);
};