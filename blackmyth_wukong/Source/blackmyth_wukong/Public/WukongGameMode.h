// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WukongGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLACKMYTH_WUKONG_API AWukongGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	// 重写(Override)父类的选择出生点函数
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

};