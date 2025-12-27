// AEnemySpawner.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AEnemySpawner.generated.h"

// 前向声明
class AEnemies;
class UBoxComponent;

// [新增] 定义每一波敌人的配置结构体
USTRUCT(BlueprintType)
struct FEnemyWaveConfig
{
    GENERATED_BODY()

    // 敌人蓝图类
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AEnemies> EnemyClass;

    // 这波敌人的总数量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
    int32 TotalCount = 1;

    // 生成间隔（秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1"))
    float SpawnInterval = 1.0f;

    // [可选] 初始延迟：游戏开始多久后才开始刷这一波
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float StartDelay = 0.0f;
};

UCLASS()
class BLACKMYTH_WUKONG_API AEnemySpawner : public AActor
{
    GENERATED_BODY()

public:
    AEnemySpawner();

protected:
    virtual void BeginPlay() override;

public:
    // ==================== [修改] 生成设置 ====================

    // [修改] 这里不再是单个变量，而是一个配置数组。
    // 在编辑器里点 "+" 号添加 3 个元素，分别配置三种敌人。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
    TArray<FEnemyWaveConfig> EnemyWaves;

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
    UBoxComponent* SpawnArea;

    // ==================== 公开函数 ====================

    // 开始所有波次的生成
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void StartAllSpawns();

private:
    // ==================== [新增] 内部逻辑变量 ====================

    // 记录每一波还剩多少个怪没刷 (索引对应 EnemyWaves)
    TArray<int32> WaveRemainingCounts;

    // 每一波独立的定时器句柄，互不干扰
    TArray<FTimerHandle> WaveTimerHandles;

    // 存储已生成的所有敌人引用
    UPROPERTY()
    TArray<AEnemies*> SpawnedEnemies;

    // ==================== 内部函数 ====================

    // 处理某一波生成的逻辑 (带参数，由定时器调用)
    void HandleWaveSpawn(int32 WaveIndex);

    // 实际执行生成一个单位
    void SpawnOneEnemy(int32 WaveIndex);

    // 获取生成位置
    FVector GetSpawnLocation();
};