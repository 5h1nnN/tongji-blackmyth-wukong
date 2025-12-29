#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_FireEnemyArrow.generated.h"

/**
 * 用于通知敌人发射远程攻击的 C++ Notify
 */
UCLASS()
class BLACKMYTH_WUKONG_API UAnimNotify_FireEnemyArrow : public UAnimNotify
{
	GENERATED_BODY()

public:
	// 构造函数
	UAnimNotify_FireEnemyArrow();

	// 重写 Notify 函数 - 这是动画播放到该帧时会自动调用的函数
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};