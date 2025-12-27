// AEnemySpawner.cpp
#include "AEnemySpawner.h"
#include "Enemies.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h" // 用于计算随机位置

AEnemySpawner::AEnemySpawner()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建根组件
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = Root;

    // 创建生成区域可视化盒子
    SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
    SpawnArea->SetupAttachment(RootComponent);
    SpawnArea->SetBoxExtent(FVector(200.f, 200.f, 50.f));
    SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SpawnArea->SetHiddenInGame(true); // 游戏中通常隐藏
    SpawnArea->ShapeColor = FColor::Green;
}

void AEnemySpawner::BeginPlay()
{
    Super::BeginPlay();

    if (bSpawnOnBeginPlay)
    {
        StartAllSpawns();
    }
}

void AEnemySpawner::StartAllSpawns()
{
    int32 NumWaves = EnemyWaves.Num();
    if (NumWaves == 0) return;

    // 1. 初始化运行时数组的大小
    WaveRemainingCounts.Init(0, NumWaves);
    WaveTimerHandles.Init(FTimerHandle(), NumWaves);

    // 2. 遍历所有配置，启动逻辑
    for (int32 i = 0; i < NumWaves; i++)
    {
        const FEnemyWaveConfig& Config = EnemyWaves[i];

        // 设置这波的初始剩余数量
        WaveRemainingCounts[i] = Config.TotalCount;

        if (Config.TotalCount > 0 && Config.EnemyClass)
        {
            // 创建一个带参数的委托：绑定 HandleWaveSpawn 函数，并传入当前的索引 i
            FTimerDelegate TimerDel;
            TimerDel.BindUObject(this, &AEnemySpawner::HandleWaveSpawn, i);

            // 如果有初始延迟 (StartDelay)，就延迟执行；否则立即执行（延迟0.1s避免卡顿）
            float FirstDelay = (Config.StartDelay > 0.f) ? Config.StartDelay : 0.1f;

            // 启动定时器
            GetWorldTimerManager().SetTimer(WaveTimerHandles[i], TimerDel, FirstDelay, false);
        }
    }
}

// 这是每一波循环逻辑的核心
void AEnemySpawner::HandleWaveSpawn(int32 WaveIndex)
{
    // 安全检查：索引有效性
    if (!EnemyWaves.IsValidIndex(WaveIndex)) return;

    // 1. 生成一个敌人
    SpawnOneEnemy(WaveIndex);

    // 2. 检查是否还需要继续生成
    if (WaveRemainingCounts[WaveIndex] > 0)
    {
        const FEnemyWaveConfig& Config = EnemyWaves[WaveIndex];

        // 再次绑定委托
        FTimerDelegate TimerDel;
        TimerDel.BindUObject(this, &AEnemySpawner::HandleWaveSpawn, WaveIndex);

        // 设置下一次生成的定时器
        GetWorldTimerManager().SetTimer(
            WaveTimerHandles[WaveIndex],
            TimerDel,
            Config.SpawnInterval, // 使用配置的时间间隔
            false
        );
    }
    else
    {
        // 这波生成完毕，可以在这里打印日志或者触发事件
        // UE_LOG(LogTemp, Log, TEXT("第 %d 波生成结束"), WaveIndex);
    }
}

void AEnemySpawner::SpawnOneEnemy(int32 WaveIndex)
{
    if (!EnemyWaves.IsValidIndex(WaveIndex)) return;

    // 获取配置引用
    const FEnemyWaveConfig& Config = EnemyWaves[WaveIndex];

    // 双重检查数量
    if (WaveRemainingCounts[WaveIndex] <= 0) return;

    UWorld* World = GetWorld();
    if (!World || !Config.EnemyClass) return;

    // 获取位置
    FVector Location = GetSpawnLocation();
    FRotator Rotation = GetActorRotation();

    // 生成参数
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 生成敌人
    AEnemies* NewEnemy = World->SpawnActor<AEnemies>(Config.EnemyClass, Location, Rotation, SpawnParams);

    if (NewEnemy)
    {
        SpawnedEnemies.Add(NewEnemy);

        // 扣减剩余数量
        WaveRemainingCounts[WaveIndex]--;

        // UE_LOG(LogTemp, Log, TEXT("生成敌人: %s (波次: %d, 剩余: %d)"), *NewEnemy->GetName(), WaveIndex, WaveRemainingCounts[WaveIndex]);
    }
}

FVector AEnemySpawner::GetSpawnLocation()
{
    FVector BaseLocation = GetActorLocation();

    if (bRandomSpawnLocation)
    {
        // 在XY平面圆形范围内随机
        FVector2D RandomPoint = FMath::RandPointInCircle(SpawnRadius);
        return BaseLocation + FVector(RandomPoint.X, RandomPoint.Y, 0.f);
    }

    return BaseLocation;
}