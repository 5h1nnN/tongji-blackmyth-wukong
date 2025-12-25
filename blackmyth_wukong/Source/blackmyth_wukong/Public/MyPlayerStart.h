#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "MyPlayerStart.generated.h"

/**
 * 自定义出生点类
 */
UCLASS()
class BLACKMYTH_WUKONG_API AMyPlayerStart : public APlayerStart
{
    GENERATED_BODY()

public:
    // 定义一个队伍ID，允许在编辑器中修改
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Rules")
    int32 SpawnTeamID;

    // 构造函数
    AMyPlayerStart(const FObjectInitializer& ObjectInitializer);
};
