// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_WeaponCollision.generated.h"

/**
 * C++ 版本的武器碰撞通知状态
 */
UCLASS()
class BLACKMYTH_WUKONG_API UANS_WeaponCollision : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    // 构造函数
    UANS_WeaponCollision();

    // 暴露给编辑器的变量：是否开启左手？
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    bool bEnableLeft;

    // 暴露给编辑器的变量：是否开启右手？
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    bool bEnableRight;

    // 重写 NotifyBegin (动画开始时)
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

    // 重写 NotifyEnd (动画结束时)
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};