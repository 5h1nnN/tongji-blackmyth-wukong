#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_FeyAttack.generated.h"

UCLASS()
class BLACKMYTH_WUKONG_API UAnimNotify_FeyAttack : public UAnimNotify
{
	GENERATED_BODY()

public:
	// ÖØÐ´ Notify º¯Êý
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};