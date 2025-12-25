 // Fill out your copyright notice in the Description page of Project Settings.


#include "SparrowProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"


ASparrowProjectile::ASparrowProjectile()
{
    PrimaryActorTick.bCanEverTick = false; // 投射物通常不需要Tick，优化性能

    // 1. 初始化碰撞球
    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    CollisionComp->InitSphereRadius(10.0f);
    CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
    // 绑定碰撞事件
    CollisionComp->OnComponentHit.AddDynamic(this, &ASparrowProjectile::OnHit);
    RootComponent = CollisionComp;

    // 2. 初始化模型
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
    ProjectileMesh->SetupAttachment(CollisionComp);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 模型不参与碰撞

    // 3. 初始化运动组件
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
    ProjectileMovement->UpdatedComponent = CollisionComp;
    ProjectileMovement->InitialSpeed = 3000.f;
    ProjectileMovement->MaxSpeed = 3000.f;
    ProjectileMovement->bRotationFollowsVelocity = true; // 箭矢方向跟随速度方向
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->ProjectileGravityScale = 0.05f; // 微弱重力

    InitialLifeSpan = 5.0f; // 5秒后自动销毁
}

void ASparrowProjectile::BeginPlay()
{
    Super::BeginPlay();

    // 获取发射者 (Owner)
    AActor* MyOwner = GetOwner();
    if (MyOwner && CollisionComp)
    {
        // 告诉碰撞组件：移动时忽略我的主人
        CollisionComp->IgnoreActorWhenMoving(MyOwner, true);
    }
}

void ASparrowProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // 排除空对象、排除自己、排除发射者(Owner)
    if ((OtherActor != nullptr) && (OtherActor != this) && (OtherActor != GetOwner()))
    {
        // 获取发射者的控制器 (用于伤害归属判定)
        AController* InstigatorCtrl = nullptr;
        if (GetOwner())
        {
            InstigatorCtrl = GetOwner()->GetInstigatorController();
        }

        // 应用伤害
        UGameplayStatics::ApplyDamage(
            OtherActor,
            DamageValue,
            InstigatorCtrl,
            this,
            UDamageType::StaticClass()
        );

        // 这里可以播放击中特效 (SpawnEmitterAtLocation) 或 音效 (PlaySoundAtLocation)

        // 销毁箭矢
        Destroy();
    }
}