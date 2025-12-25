// Fill out your copyright notice in the Description page of Project Settings.


#include "WukongGameMode.h"
#include "MyPlayerStart.h"        // 引用我们刚才写的出生点类
#include "EngineUtils.h"          // 引用迭代器工具，用于查找场景中的Actor
#include "GameFramework/PlayerState.h" 

AActor* AWukongGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    // 假设我们要找 TeamID 为 1 的出生点 (实际项目中，这个ID应该从 Player->PlayerState 获取)
    int32 RequiredTeamID = 1;

    // 使用 TActorIterator 遍历场景中所有的 "AMyPlayerStart"
    for (TActorIterator<AMyPlayerStart> It(GetWorld()); It; ++It)
    {
        AMyPlayerStart* CurrentSpawnPoint = *It;
        // 如果找到了一个出生点，且它的 ID 匹配我们需要的值
        if (CurrentSpawnPoint && CurrentSpawnPoint->SpawnTeamID == RequiredTeamID)
        {
            // 直接返回这个出生点
            return CurrentSpawnPoint;
        }
    }

    // 如果上面没找到合适的（比如场景里忘放了），就调用父类默认方法，随便找一个普通的 PlayerStart
    return Super::ChoosePlayerStart_Implementation(Player);
}