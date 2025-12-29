#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SparrowProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;

UCLASS()
class BLACKMYTH_WUKONG_API ASparrowProjectile : public AActor
{
    GENERATED_BODY()

public:
    ASparrowProjectile();

protected:
    virtual void BeginPlay() override;

public:
    // 碰撞组件 (根组件)
    UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
    USphereComponent* CollisionComp;

    // 箭矢模型
    UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
    UStaticMeshComponent* ProjectileMesh;

    // 运动组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
    UProjectileMovementComponent* ProjectileMovement;

    // 基础伤害值 (可以被生成者修改)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float DamageValue = 20.0f;

    // 碰撞回调
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};