// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyHealthBar.h" // 引入基类头文件
#include "Components/TextBlock.h" // 引入文本组件
#include "BossHealthBar.generated.h"

/**
 * 继承自 UEnemyHealthBar，专门用于 Boss 屏幕 UI
 */
UCLASS()
class BLACKMYTH_WUKONG_API UBossHealthBar : public UEnemyHealthBar
{
	GENERATED_BODY()

public:
	// 设置 Boss 名字
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetBossName(FText Name);

protected:
	// [新增] 绑定 Boss 名字文本控件
	// 注意：父类的 HealthProgressBar 会自动继承，不需要在这里重新声明
	UPROPERTY(meta = (BindWidget))
	UTextBlock* BossNameText;
};