// AEnemySpawner.cpp

#include "AEnemySpawner.h"
#include "Enemies.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"

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
    SpawnArea->SetHiddenInGame(true);
    SpawnArea->ShapeColor = FColor::Green;
}

void AEnemySpawner::BeginPlay()
{
    Super::BeginPlay();

    if (bSpawnOnBeginPlay)
    {
        SpawnAllEnemies();
    }
}

AEnemies* AEnemySpawner::SpawnEnemy()
{
    // 检查敌人类是否设置
    if (!EnemyClass)
    {
        UE_LOG(LogTemp, Error, TEXT("EnemySpawner: EnemyClass 未设置!"));
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // 获取生成位置和旋转
    FVector Location = GetSpawnLocation();
    FRotator Rotation = GetActorRotation();

    // 设置生成参数
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 生成敌人
    AEnemies* NewEnemy = World->SpawnActor<AEnemies>(EnemyClass, Location, Rotation, SpawnParams);

    if (NewEnemy)
    {
        SpawnedEnemies.Add(NewEnemy);
        UE_LOG(LogTemp, Log, TEXT("成功生成敌人: %s"), *NewEnemy->GetName());
    }

    return NewEnemy;
}

void AEnemySpawner::SpawnAllEnemies()
{
    CurrentSpawnedCount = 0;

    // 清理之前的定时器
    GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

    if (SpawnCount <= 0)
    {
        return;
    }

    if (SpawnInterval <= 0.f)
    {
        // 一次性生成所有敌人
        for (int32 i = 0; i < SpawnCount; i++)
        {
            SpawnEnemy();
        }
    }
    else
    {
        // 使用定时器间隔生成
        GetWorldTimerManager().SetTimer(
            SpawnTimerHandle,
            this,
            &AEnemySpawner::OnSpawnTimerTick,
            SpawnInterval,
            true,
            0.f
        );
    }
}

void AEnemySpawner::OnSpawnTimerTick()
{
    SpawnEnemy();
    CurrentSpawnedCount++;

    if (CurrentSpawnedCount >= SpawnCount)
    {
        GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
    }
}

FVector AEnemySpawner::GetSpawnLocation()
{
    FVector BaseLocation = GetActorLocation();

    if (bRandomSpawnLocation)
    {
        float RandomX = FMath::RandRange(-SpawnRadius, SpawnRadius);
        float RandomY = FMath::RandRange(-SpawnRadius, SpawnRadius);
        return BaseLocation + FVector(RandomX, RandomY, 0.f);
    }

    return BaseLocation;
}