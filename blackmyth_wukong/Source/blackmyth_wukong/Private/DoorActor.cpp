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

			// 开启输入权限，否则蓝图里的按键事件也不会响应
			EnableInput(PC);

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

// 3. 可以被蓝图调用
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


	// 1. 将传入的 Pawn 转换为自定义角色类
	// 这里的 InstigatorPawn 就是触发交互的玩家
	if (Ablackmyth_wukongCharacter* MyChar = Cast<Ablackmyth_wukongCharacter>(InstigatorPawn))
	{
		// 2. 获取自定义的游戏实例 (GameInstance)
		if (UMyGameInstance* MyGI = Cast<UMyGameInstance>(GetGameInstance()))
		{
			// 3. 将角色身上的数据“上传”给 GameInstance
			MyGI->SavedMaxHealth = MyChar->MaxHealth;
			MyGI->SavedXP = MyChar->CurrentXP;
			MyGI->SavedLevel = MyChar->CharacterLevel;

		}
	}


	// 4. 执行跳转
	FString Msg = FString::Printf(TEXT("Loading Level: %s"), *TargetLevelName.ToString());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Msg);

	UGameplayStatics::OpenLevel(this, TargetLevelName);
}