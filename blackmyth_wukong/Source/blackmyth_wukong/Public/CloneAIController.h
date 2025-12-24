#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "CloneAIController.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
class UAISenseConfig_Sight;

UCLASS()
class BLACKMYTH_WUKONG_API ACloneAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACloneAIController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	// --- 组件 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UBehaviorTreeComponent* BehaviorTreeComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UBlackboardComponent* BlackboardComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAISenseConfig_Sight* SightConfig;

	// --- 配置 ---
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	class UBehaviorTree* BehaviorTreeAsset;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	FName TargetKeyName = "TargetActor"; // 对应黑板中的 Object Key

	// --- 感知回调 ---
	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);

public:
	// --- 接口 (供行为树调用) ---

	// 尝试攻击目标。返回 true 表示距离足够并执行了攻击，返回 false 表示距离太远需要追逐
	UFUNCTION(BlueprintCallable, Category = "AI")
	bool TryAttackTarget();

	// 获取当前锁定的目标
	UFUNCTION(BlueprintPure, Category = "AI")
	AActor* GetTargetEnemy() const;
};