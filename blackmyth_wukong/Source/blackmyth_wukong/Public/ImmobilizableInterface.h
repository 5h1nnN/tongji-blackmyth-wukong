#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ImmobilizableInterface.generated.h"

UINTERFACE(MinimalAPI)
class UImmobilizableInterface : public UInterface
{
	GENERATED_BODY()
};

class BLACKMYTH_WUKONG_API IImmobilizableInterface
{
	GENERATED_BODY()

public:
	// 触发定身
	// Duration: 定身持续时间
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void OnImmobilized(float Duration);

	// 解除定身
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void OnUnImmobilized();
};