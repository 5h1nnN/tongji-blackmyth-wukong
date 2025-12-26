#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionInterface.h" 
#include "DoorActor.generated.h"

UCLASS()
class BLACKMYTH_WUKONG_API ADoorActor : public AActor, public IInteractionInterface
{
	GENERATED_BODY()

public:
	ADoorActor();

protected:
	virtual void BeginPlay() override;

public:
	// --- 组件 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* TriggerBox;

	// --- 变量 ---
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Level Config")
	FName TargetLevelName;

	// --- 函数 ---

	// 1. 重叠检测
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 2. 【关键】暴露给蓝图调用的函数
	// BlueprintCallable 让蓝图可以呼叫这个 C++ 函数
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TriggerInteraction();

	// 3. 接口实现
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
};