// Fill out your copyright notice in the Description page of Project Settings.


#include "AN_FireEnemyArrow.h"
#include "Enemies.h" // 引用敌人头文件

UAnimNotify_FireEnemyArrow::UAnimNotify_FireEnemyArrow()
{
    // 设置编辑器显示的颜色
    NotifyColor = FColor(255, 0, 0, 255); // 红色
}

void UAnimNotify_FireEnemyArrow::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    // 1. 安全检查
    if (!MeshComp) return;

    // 2. 获取 Mesh 的所有者 (Owner)
    AActor* OwnerActor = MeshComp->GetOwner();
    if (!OwnerActor) return;

    // 3. 尝试将 Owner 转换为我们的敌人 C++ 类 (AEnemies)
    AEnemies* EnemyCharacter = Cast<AEnemies>(OwnerActor);
    if (EnemyCharacter)
    {
        // 4. 只有当转换成功时，才调用发射函数
        // 确保敌人没死
        if (!EnemyCharacter->IsDead())
        {
            EnemyCharacter->FireRangedAttack();
        }
    }
}