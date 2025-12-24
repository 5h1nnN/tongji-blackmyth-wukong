#include "CloneAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "blackmyth_wukong/blackmyth_wukongCharacter.h"
#include "InputActionValue.h" 

ACloneAIController::ACloneAIController()
{
	// 1. 初始化组件
	BehaviorTreeComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComp"));
	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
	SetPerceptionComponent(*CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp")));

	// 2. 配置视觉
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	if (SightConfig)
	{
		SightConfig->SightRadius = 1500.0f;
		SightConfig->LoseSightRadius = 2000.0f;
		SightConfig->PeripheralVisionAngleDegrees = 180.0f; // 360度感知
		SightConfig->SetMaxAge(5.0f);

		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

		GetPerceptionComponent()->ConfigureSense(*SightConfig);
		GetPerceptionComponent()->SetDominantSense(SightConfig->GetSenseImplementation());
	}
}

void ACloneAIController::BeginPlay()
{
	Super::BeginPlay();
	if (GetPerceptionComponent())
	{
		GetPerceptionComponent()->OnTargetPerceptionUpdated.AddDynamic(this, &ACloneAIController::OnTargetDetected);
	}
}

void ACloneAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn && BehaviorTreeAsset)
	{
		BlackboardComp->InitializeBlackboard(*BehaviorTreeAsset->BlackboardAsset);
		BehaviorTreeComp->StartTree(*BehaviorTreeAsset);
	}
}

void ACloneAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{

	// --- 调试日志 (英文版，防止编译报错) ---
	FString Info = FString::Printf(TEXT("AI Perception: Target[%s] | Sensed[%s] | TagMatch[%s]"),
		*Actor->GetName(),
		Stimulus.WasSuccessfullySensed() ? TEXT("YES") : TEXT("NO"),
		Actor->ActorHasTag(FName("Enemy")) ? TEXT("YES") : TEXT("NO")
	);

	// 打印红色日志到屏幕
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, Info);
	// --- 调试日志结束 ---

	// 简单逻辑：只要看到带有 Enemy 标签的，就锁定为目标
	if (Stimulus.WasSuccessfullySensed())
	{
		if (Actor->ActorHasTag(FName("Enemy")))
		{
			GetBlackboardComponent()->SetValueAsObject(TargetKeyName, Actor);
		}
	}
	else
	{
		// 丢失视野时不立即清除，让 AI 跑到最后看到的位置（BehaviorTree 处理）
		// 如果需要立即清除，取消下面注释：
		// if (GetTargetEnemy() == Actor) GetBlackboardComponent()->ClearValue(TargetKeyName);
	}
}

AActor* ACloneAIController::GetTargetEnemy() const
{
	if (BlackboardComp)
	{
		return Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKeyName));
	}
	return nullptr;
}

bool ACloneAIController::TryAttackTarget()
{
	Ablackmyth_wukongCharacter* MyChar = Cast<Ablackmyth_wukongCharacter>(GetPawn());
	AActor* Target = GetTargetEnemy();

	if (MyChar && Target)
	{
		float Distance = FVector::Dist(MyChar->GetActorLocation(), Target->GetActorLocation());

		// 攻击范围判定 (例如 220 厘米)
		if (Distance <= 220.0f)
		{
			// 1. 瞬间转向敌人 (简化版，平滑转向可在 BT 中做)
			FVector Direction = (Target->GetActorLocation() - MyChar->GetActorLocation()).GetSafeNormal();
			Direction.Z = 0;
			MyChar->SetActorRotation(Direction.Rotation());

			// 2. 触发攻击
			FInputActionValue DummyValue;
			MyChar->PerformLightAttack(DummyValue);
			return true; // 攻击成功
		}
	}
	return false; // 距离太远或无目标
}