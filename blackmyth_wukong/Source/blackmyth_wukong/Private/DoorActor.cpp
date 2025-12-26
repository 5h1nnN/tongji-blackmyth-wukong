#include "DoorActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
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
	if (TargetLevelName.IsNone())
	{
		return;
	}

	FString Msg = FString::Printf(TEXT("Loading Level: %s"), *TargetLevelName.ToString());

	UGameplayStatics::OpenLevel(this, TargetLevelName);
}