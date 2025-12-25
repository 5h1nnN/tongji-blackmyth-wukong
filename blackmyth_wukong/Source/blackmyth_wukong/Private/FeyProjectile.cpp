#include "FeyProjectile.h"
#include "NiagaraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
AFeyProjectile::AFeyProjectile()
{
	// 在构造函数里调整 Fey 特有的属性
	DamageAmount = 25.0f; // Fey 的攻击力高一些
	ProjectileMovement->InitialSpeed = 3500.f;
	ProjectileMovement->MaxSpeed = 3500.f;

	// 添加 Fey 特有的拖尾特效
	TrailEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(RootComponent);
}