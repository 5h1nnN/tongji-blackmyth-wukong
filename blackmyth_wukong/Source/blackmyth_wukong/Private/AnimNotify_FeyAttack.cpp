#include "AnimNotify_FeyAttack.h"
#include "FeyCharacter.h"

void UAnimNotify_FeyAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		// 尝试转换 Owner 为 FeyCharacter
		AFeyCharacter* FeyChar = Cast<AFeyCharacter>(MeshComp->GetOwner());
		if (FeyChar)
		{
			// 直接调用角色类中的发射函数
			FeyChar->ExecuteSpawnProjectile();
		}
	}
}