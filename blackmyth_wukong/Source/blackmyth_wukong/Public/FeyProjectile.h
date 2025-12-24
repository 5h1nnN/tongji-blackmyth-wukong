#pragma once

#include "CoreMinimal.h"
#include "BaseProjectile.h"
#include "FeyProjectile.generated.h"

UCLASS()
class BLACKMYTH_WUKONG_API AFeyProjectile : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AFeyProjectile();

protected:
	/** Fey子弹特有的组件，比如拖尾烟雾 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	class UNiagaraComponent* TrailEffect;

	// 如果 Fey 的子弹有特殊逻辑（比如追踪），可以重写 OnHit 或 Tick
};