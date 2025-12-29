#include "ANS_WeaponCollision.h"
#include "Enemies.h"
UANS_WeaponCollision::UANS_WeaponCollision()
{
    // 默认右手开启，左手关闭
    bEnableRight = true;
    bEnableLeft = false;
}

void UANS_WeaponCollision::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    // 1. 检查网格体是否存在
    if (!MeshComp) return;

    // 2. 获取拥有者并转换为 AEnemies
    if (AEnemies* Enemy = Cast<AEnemies>(MeshComp->GetOwner()))
    {
        // 3. 调用 C++ 里的开启函数，传入左右手参数
        Enemy->EnableWeaponCollision(bEnableLeft, bEnableRight);
    }
}

void UANS_WeaponCollision::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    // 1. 检查网格体
    if (!MeshComp) return;

    // 2. 获取拥有者并转换为 AEnemies
    if (AEnemies* Enemy = Cast<AEnemies>(MeshComp->GetOwner()))
    {
        // 3. 调用 C++ 里的关闭函数
        Enemy->DisableWeaponCollision();
    }
}

