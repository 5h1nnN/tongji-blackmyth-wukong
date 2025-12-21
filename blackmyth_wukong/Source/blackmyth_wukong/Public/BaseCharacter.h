#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

UCLASS()
class BLACKMYTH_WUKONG_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseCharacter();

	// 在蓝图中指定要变身的目标类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	TSubclassOf<ABaseCharacter> TargetCharacterClass;

	// 变身特效 (Niagara 或 粒子)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	class UNiagaraSystem* TransformationVFX;

protected:
	// 变身核心函数
	UFUNCTION(BlueprintCallable, Category = "Transformation")
	void TransformCharacter();

	// 增强输入相关
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* TransformAction;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};