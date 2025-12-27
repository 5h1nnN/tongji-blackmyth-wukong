#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h" // 继承自 BaseCharacter
#include "InputActionValue.h"
#include "FeyCharacter.generated.h"

// 前向声明
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UAnimMontage;
class ABaseProjectile;

UCLASS()
class BLACKMYTH_WUKONG_API AFeyCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AFeyCharacter();

	UFUNCTION(BlueprintCallable)
	void ExecuteSpawnProjectile();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** 自动变回原型的时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	float AutoTransformDuration = 10.0f;

	FTimerHandle TransformTimerHandle;
	void OnAutoTransformTimerTimeout();

	/** 摄像机组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** --- Fey 特有的增强输入 --- */
	// 注意：父类已经有一个 DefaultMappingContext，这里我们定义 Fey 专用的
	// 如果你想让 Fey 使用完全不同的按键，可以在这里配置
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* FeyMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* AttackAction;

	/** 飞行物蓝图类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TSubclassOf<class ABaseProjectile> ProjectileClass;

	/** 攻击动画蒙太奇 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* AttackMontage;

	void Attack();

	// 输入回调
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
};