// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyHealthBar.h"

void UEnemyHealthBar::UpdateHealthPercent(float Percent)
{
    if (HealthProgressBar)
    {
        HealthProgressBar->SetPercent(Percent);
    }
}
