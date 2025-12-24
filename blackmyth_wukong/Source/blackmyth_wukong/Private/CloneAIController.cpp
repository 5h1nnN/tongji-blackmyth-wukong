#include "CloneAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "blackmyth_wukong/blackmyth_wukongCharacter.h"
#include "InputActionValue.h" // 引用输入值结构

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
		SightConfig->SightRadius = 1500.0f;           // 15米视野
		SightConfig->LoseSightRadius = 2000.0f;
		SightConfig->PeripheralVisionAngleDegrees = 180.0f; // 360度全方位感知(防止怪物在背后挨打不还手)
		SightConfig->SetMaxAge(5.0f);

		// 必须开启这三个，否则感知不到任何东西
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

	// 运行行为树
	if (InPawn && BehaviorTreeAsset)
	{
		BlackboardComp->InitializeBlackboard(*BehaviorTreeAsset->BlackboardAsset);
		BehaviorTreeComp->StartTree(*BehaviorTreeAsset);
	}
}

void ACloneAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	// 核心逻辑：只锁定带有 "TeamMonster" 标签的敌人
	if (Stimulus.WasSuccessfullySensed())
	{
		// 如果看到的东西是 Character 且 带有 TeamMonster 标签
		if (Actor->ActorHasTag(FName("TeamMonster")))
		{
			// 锁定目标
			GetBlackboardComponent()->SetValueAsObject(TargetKeyName, Actor);
		}
	}
	else
	{
		// 丢失视野逻辑 (可选：是否清除目标)
		// 简单的做法是：如果当前目标就是丢失的这个，清除它
		if (GetBlackboardComponent()->GetValueAsObject(TargetKeyName) == Actor)
		{
			GetBlackboardComponent()->ClearValue(TargetKeyName);
		}
	}
}

void ACloneAIController::TryAttackTarget()
{
	Ablackmyth_wukongCharacter* MyChar = Cast<Ablackmyth_wukongCharacter>(GetPawn());
	AActor* Target = Cast<AActor>(GetBlackboardComponent()->GetValueAsObject(TargetKeyName));

	if (MyChar && Target)
	{
		// 计算距离，如果在攻击范围内 (例如 200)
		float Distance = FVector::Dist(MyChar->GetActorLocation(), Target->GetActorLocation());
		if (Distance <= 200.0f)
		{
			// 面向敌人
			FVector Direction = (Target->GetActorLocation() - MyChar->GetActorLocation()).GetSafeNormal();
			FRotator LookRot = Direction.Rotation();
			MyChar->SetActorRotation(FRotator(0, LookRot.Yaw, 0));

			// 触发轻攻击 (模拟按键)
			FInputActionValue DummyValue;
			MyChar->PerformLightAttack(DummyValue);
		}
	}
}