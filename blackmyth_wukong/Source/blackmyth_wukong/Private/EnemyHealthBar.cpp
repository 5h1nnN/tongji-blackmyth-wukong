#include "EnemyHealthBar.h"

void UEnemyHealthBar::UpdateHealthPercent(float Percent)
{
    if (HealthProgressBar)
    {
        HealthProgressBar->SetPercent(Percent);
    }
}
