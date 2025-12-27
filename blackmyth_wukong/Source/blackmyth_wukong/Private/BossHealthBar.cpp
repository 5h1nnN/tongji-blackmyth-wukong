// Fill out your copyright notice in the Description page of Project Settings.


#include "BossHealthBar.h"

void UBossHealthBar::SetBossName(FText Name)
{
	if (BossNameText)
	{
		BossNameText->SetText(Name);
	}
}

