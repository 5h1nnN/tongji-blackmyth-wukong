#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MyGameInstance.generated.h"

UCLASS()
class BLACKMYTH_WUKONG_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	float SavedMaxHealth = -1.0f; // 默认 -1 代表未保存过

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	float SavedXP = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	int32 SavedLevel = 1;
};
