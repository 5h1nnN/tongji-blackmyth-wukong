// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h" // 引入进度条组件
#include "EnemyHealthBar.generated.h"

UCLASS()
class BLACKMYTH_WUKONG_API UEnemyHealthBar : public UUserWidget
{
    GENERATED_BODY()

public:
    // 绑定蓝图中的进度条控件
    // meta = (BindWidget) 意味着：你在蓝图里必须有个名字一模一样的进度条，否则报错
    UPROPERTY(meta = (BindWidget))
    UProgressBar* HealthProgressBar;

    // 更新血量的函数
    void UpdateHealthPercent(float Percent);
};