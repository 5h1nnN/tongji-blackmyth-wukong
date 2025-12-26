#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseFood.generated.h"

// 前置声明
class UWidgetComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class BLACKMYTH_WUKONG_API ABaseFood : public AActor
{
    GENERATED_BODY()

public:
    ABaseFood();

    // 1. 模型
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* FoodMesh;

    // 2. 检测范围
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* DetectRange;

    // 3. UI 提示 (Widget Component)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* TipWidget;

    // 4. 核心功能接口 (BlueprintNativeEvent 允许 C++ 有默认功能，也允许蓝图重写)
    // 参数传入 TargetCharacter，这样我们就知道是谁吃了苹果
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void ApplyEffect(class Ablackmyth_wukongCharacter* TargetCharacter);
};
