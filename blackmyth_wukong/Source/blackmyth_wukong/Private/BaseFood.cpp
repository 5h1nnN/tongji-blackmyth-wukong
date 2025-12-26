#include "BaseFood.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

ABaseFood::ABaseFood()
{
    PrimaryActorTick.bCanEverTick = true;

    // 创建模型
    FoodMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FoodMesh"));
    RootComponent = FoodMesh;

    // 创建范围
    DetectRange = CreateDefaultSubobject<USphereComponent>(TEXT("DetectRange"));
    DetectRange->SetupAttachment(RootComponent);
    DetectRange->SetSphereRadius(150.0f);

    // 创建 UI
    TipWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("TipWidget"));
    TipWidget->SetupAttachment(RootComponent);
    TipWidget->SetWidgetSpace(EWidgetSpace::Screen); // 设为屏幕空间
    TipWidget->SetDrawAtDesiredSize(true);           // 自适应大小
    TipWidget->SetRelativeLocation(FVector(0, 0, 100)); // 向上偏移
    TipWidget->SetHiddenInGame(true);                // 默认隐藏
}

// 默认实现：如果不重写，默认就是销毁自己
void ABaseFood::ApplyEffect_Implementation(class Ablackmyth_wukongCharacter* TargetCharacter)
{
    Destroy();
}