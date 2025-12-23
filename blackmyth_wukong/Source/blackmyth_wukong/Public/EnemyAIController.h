// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h" // 引用感知类型
#include "EnemyAIController.generated.h"

// 前置声明
class UBehaviorTreeComponent;
class UBlackboardComponent;
class UAISenseConfig_Sight;

UCLASS()
class BLACKMYTH_WUKONG_API AEnemyAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyAIController();

protected:
    // --- 1. 核心 AI 组件 ---
    // 行为树组件 (虽然 AAIController 自带 BrainComponent，但显式声明更清晰)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UBehaviorTreeComponent* BehaviorTreeComp;

    // 黑板组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UBlackboardComponent* BlackboardComp;

    // --- 2. 感知组件配置 ---
    // 视觉配置
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAISenseConfig_Sight* SightConfig;

    // --- 3. 暴露给蓝图的资产插槽 ---
    // 让我们在蓝图里选具体用哪个行为树 (BT_Grux)
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    UBehaviorTree* BehaviorTree;

    // 黑板 Key 的名字 (方便修改)
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    FName TargetActorKeyName = "TargetActor";

protected:
    virtual void BeginPlay() override;

    // 当控制器占有 Pawn 时调用 (启动 AI 的地方)
    virtual void OnPossess(APawn* InPawn) override;

    // --- 4. 感知回调函数 ---
    UFUNCTION()
    void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);

public:
    // --- 5. 硬直控制 (供 Character 调用) ---
    void HandleHitStun(float Duration);

private:
    // 硬直恢复定时器
    FTimerHandle StunTimerHandle;
    void RecoverFromStun();
};