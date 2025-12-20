// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Enemies.generated.h"

UCLASS()
class BLACKMYTH_WUKONG_API AEnemies : public ACharacter
{
    GENERATED_BODY()

public:
    // 构造函数
    AEnemies();

    // --- 在这里添加核心函数 ---

    // 重写系统的受击函数，处理扣血逻辑
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

protected:
    // 开始游戏时调用
    virtual void BeginPlay() override;

    // --- 在这里添加核心属性 ---

    // 当前血量：允许在编辑器修改，并在蓝图中读写
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat | Stats")
    float Health = 100.f;

    // 最大血量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat | Stats")
    float MaxHealth = 100.f;

    // 敌人名称（用于显示在血条上）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat | Stats")
    FText EnemyName;

public:
    // 每一帧调用
    virtual void Tick(float DeltaTime) override;

    // 绑定输入（敌人通常不需要，可以保留为空）
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};