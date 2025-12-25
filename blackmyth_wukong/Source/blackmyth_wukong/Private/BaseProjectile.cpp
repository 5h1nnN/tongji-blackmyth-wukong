#include "BaseProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

ABaseProjectile::ABaseProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 设置碰撞
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(10.0f);
	CollisionComp->SetNotifyRigidBodyCollision(true);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore); // 默认忽略
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block); // 撞动态物体
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // 阻挡敌人
	CollisionComp->OnComponentHit.AddDynamic(this, &ABaseProjectile::OnHit);
	RootComponent = CollisionComp;

	// 2. 设置移动
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 2000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f; // 默认无重力

	// 3. 设置模型
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InitialLifeSpan = 5.0f;
}

void ABaseProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 1. 获取发射者
	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		// 2. 让碰撞组件忽略掉发射者
		CollisionComp->IgnoreActorWhenMoving(MyOwner, true);

		// 如果 MyOwner 是角色，可能还需要忽略它的子组件
		MyOwner->SetInstigator(Cast<APawn>(MyOwner));
	}

	// 强制开启命中事件
	CollisionComp->SetNotifyRigidBodyCollision(true);
}

void ABaseProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor)
	{
		// 打印撞到的所有东西
		UE_LOG(LogTemp, Warning, TEXT("Projectile Hit: %s"), *OtherActor->GetName());
	}

	if ((OtherActor != nullptr) && (OtherActor != this) && (OtherActor != GetOwner()))
	{
		// 1. 应用伤害
		UGameplayStatics::ApplyDamage(OtherActor, DamageAmount, GetInstigatorController(), this, UDamageType::StaticClass());

		// 2. 播放特效
		if (ImpactEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, Hit.ImpactPoint);
		}

		// 3. 销毁
		Destroy();
	}
}