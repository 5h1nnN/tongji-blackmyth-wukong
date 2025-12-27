#include "DoorActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MyGameInstance.h"       // 你的 GameInstance 头文件
#include "../blackmyth_wukongCharacter.h"
#include "GameFramework/PlayerController.h"

// 构造函数
ADoorActor::ADoorActor()
{
	PrimaryActorTick.bCanEverTick = false;

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	RootComponent = DoorMesh;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(DoorMesh);
	TriggerBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

void ADoorActor::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ADoorActor::OnOverlapBegin);
		TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ADoorActor::OnOverlapEnd);
	}
}

// 1. 玩家进入范围
void ADoorActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC && OtherActor == PC->GetPawn())
		{

			// 【关键】必须开启输入权限，否则蓝图里的按键事件也不会响应
			EnableInput(PC);

			// 注意：这里没有任何 BindAction 的代码了！
		}
	}
}

// 2. 玩家离开范围
void ADoorActor::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor != this)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC && OtherActor == PC->GetPawn())
		{
			DisableInput(PC);
		}
	}
}

// 3. 这是一个可以被蓝图调用的“扳机”
void ADoorActor::TriggerInteraction()
{

	APawn* PlayerPawn = nullptr;
	if (GetWorld())
	{
		PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	}
	// 调用真正的逻辑
	Interact_Implementation(PlayerPawn);
}

// 4. 核心跳转逻辑
void ADoorActor::Interact_Implementation(APawn* InstigatorPawn)
{
	// 0. 安全检查：如果没有设置关卡名，直接返回
	if (TargetLevelName.IsNone())
	{
		return;
	}

	// --- 新增保存逻辑开始 ---

	// 1. 将传入的 Pawn 转换为你的自定义角色类
	// 注意：这里的 InstigatorPawn 就是触发交互的玩家
	if (Ablackmyth_wukongCharacter* MyChar = Cast<Ablackmyth_wukongCharacter>(InstigatorPawn))
	{
		// 2. 获取自定义的游戏实例 (GameInstance)
		if (UMyGameInstance* MyGI = Cast<UMyGameInstance>(GetGameInstance()))
		{
			// 3. 将角色身上的数据“上传”给 GameInstance
			// 请确保你的变量名与下面的一致，如果不一致请手动修改
			MyGI->SavedMaxHealth = MyChar->MaxHealth; // 假设角色里叫 CurrentHealth
			MyGI->SavedXP = MyChar->CurrentXP;     // 假设角色里叫 CurrentXP
			MyGI->SavedLevel = MyChar->CharacterLevel;  // 假设角色里叫 CurrentLevel

		}
	}

	// --- 新增保存逻辑结束 ---

	// 4. 执行跳转
	FString Msg = FString::Printf(TEXT("Loading Level: %s"), *TargetLevelName.ToString());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Msg);

	UGameplayStatics::OpenLevel(this, TargetLevelName);
}