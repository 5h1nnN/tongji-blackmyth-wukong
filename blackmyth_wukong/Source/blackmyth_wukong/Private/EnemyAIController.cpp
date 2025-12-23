// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "GameFramework/Character.h"

AEnemyAIController::AEnemyAIController()
{
    // 1. 创建组件
    BehaviorTreeComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComp"));
    BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));

    // 初始化感知组件 (父类 AAIController 可能没有初始化它，我们需要手动创建)
    SetPerceptionComponent(*CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp")));

    // 2. 配置视觉 (Sight Config)
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

    if (SightConfig)
    {
        SightConfig->SightRadius = 2000.0f;           // 视距 20米
        SightConfig->LoseSightRadius = 2500.0f;       // 丢失视距
        SightConfig->PeripheralVisionAngleDegrees = 90.0f; // 视野角度 180度
        SightConfig->SetMaxAge(5.0f);                 // 记忆时间

        // 关键：检测中立阵营 (玩家通常是 Neutral)
        SightConfig->DetectionByAffiliation.bDetectEnemies = true;
        SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
        SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

        // 将配置应用到感知组件
        GetPerceptionComponent()->ConfigureSense(*SightConfig);
        GetPerceptionComponent()->SetDominantSense(SightConfig->GetSenseImplementation());
    }
}

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    // 绑定感知更新事件
    if (GetPerceptionComponent())
    {
        GetPerceptionComponent()->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetDetected);
    }
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // 启动 AI
    if (InPawn && BehaviorTree)
    {
        // 1. 初始化黑板
        if (BehaviorTree->BlackboardAsset)
        {
            BlackboardComp->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
        }

        // 2. 运行行为树
        BehaviorTreeComp->StartTree(*BehaviorTree);
    }
}

// 对应蓝图中的 "On Target Perception Updated"
void AEnemyAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
    // 过滤：只关注 Character 类型 (即玩家)
    if (auto* SensedCharacter = Cast<ACharacter>(Actor))
    {
        // Stimulus.WasSuccessfullySensed() 相当于蓝图里的 Successfully Sensed 布尔值
        if (Stimulus.WasSuccessfullySensed())
        {
            // 看见了 -> 写入黑板
            GetBlackboardComponent()->SetValueAsObject(TargetActorKeyName, SensedCharacter);

            // 调试打印
            // if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("看见玩家了！"));
        }
        else
        {
            // 丢失视野 -> 清空黑板
            GetBlackboardComponent()->ClearValue(TargetActorKeyName);

            // 调试打印
            // if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("跟丢了！"));
        }
    }
}

// 处理硬直 (替代之前的蓝图 Implementable Event)
void AEnemyAIController::HandleHitStun(float Duration)
{
    // 1. 停止移动
    StopMovement();

    // 2. 停止行为树逻辑 (Reason: "HitStun")
    if (BehaviorTreeComp)
    {
        BehaviorTreeComp->StopLogic("HitStun");
    }

    // 3. 设置定时器恢复
    GetWorld()->GetTimerManager().SetTimer(StunTimerHandle, this, &AEnemyAIController::RecoverFromStun, Duration, false);
}

void AEnemyAIController::RecoverFromStun()
{
    // 恢复行为树逻辑
    if (BehaviorTreeComp)
    {
        BehaviorTreeComp->RestartLogic();
    }
}

