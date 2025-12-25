#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h" // 保持继承关系
#include "InputActionValue.h" // 必须包含
#include "FeyCharacter.generated.h"

// 前向声明
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UAnimMontage;
class UAnimSequence;
class UUserWidget; // [新增] 必须声明，否则识别不了 UI 类
class ABaseProjectile;
struct FInputActionValue;

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
	/** 自动变回原型的时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	float AutoTransformDuration = 10.0f;

	/** 定时器句柄 */
	FTimerHandle TransformTimerHandle;

	/** 定时器到期后执行的包装函数 */
	void OnAutoTransformTimerTimeout();
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;


	/** --- 增强输入资源 --- */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* FeyMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* AttackAction;

	/** 飞行物蓝图类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TSubclassOf<class ABaseProjectile> ProjectileClass;

	/** 攻击动画蒙太奇 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* AttackMontage;

	/** 攻击函数 */
	void Attack();
	/** --- 输入处理函数 --- */
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);


};