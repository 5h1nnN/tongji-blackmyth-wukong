// AEnemySpawner.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AEnemySpawner.generated.h"

// 前向声明
class AEnemies;
class UBoxComponent;

UCLASS()
class BLACKMYTH_WUKONG_API AEnemySpawner : public AActor
{
    GENERATED_BODY()

public:
    AEnemySpawner();

protected:
    virtual void BeginPlay() override;

public:
    // ==================== 生成设置 ====================

    // 要生成的敌人蓝图类（在编辑器中选择你的敌人蓝图）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    TSubclassOf<AEnemies> EnemyClass;

    // 生成数量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    int32 SpawnCount = 1;

    // 生成间隔（秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    float SpawnInterval = 0.5f;

    // 是否在游戏开始时自动生成
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    bool bSpawnOnBeginPlay = true;

    // 是否在随机位置生成
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    bool bRandomSpawnLocation = false;

    // 随机生成范围
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    float SpawnRadius = 200.f;

    // ==================== 可视化组件 ====================

    // 生成区域可视化
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
    UBoxComponent* SpawnArea;

    // ==================== 公开函数 ====================

    // 生成单个敌人
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    AEnemies* SpawnEnemy();

    // 生成多个敌人
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void SpawnAllEnemies();

private:
    // 已生成的数量计数
    int32 CurrentSpawnedCount = 0;

    // 定时器句柄
    FTimerHandle SpawnTimerHandle;

    // 存储已生成的敌人
    UPROPERTY()
    TArray<AEnemies*> SpawnedEnemies;

    // 定时器回调
    void OnSpawnTimerTick();

    // 获取生成位置
    FVector GetSpawnLocation();
};