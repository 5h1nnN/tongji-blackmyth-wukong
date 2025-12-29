#include "BossHealthBar.h"

void UBossHealthBar::SetBossName(FText Name)
{
	if (BossNameText)
	{
		BossNameText->SetText(Name);
	}
}
