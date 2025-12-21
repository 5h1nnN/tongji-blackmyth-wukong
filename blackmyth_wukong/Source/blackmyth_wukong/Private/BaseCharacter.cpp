#include "BaseCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABaseCharacter::TransformCharacter()
{
	if (!TargetCharacterClass) return;

	UWorld* World = GetWorld();
	if (World)
	{
		// 1. 获取当前位置和旋转
		FVector Location = GetActorLocation();
		FRotator Rotation = GetActorRotation();
		FTransform SpawnTransform = GetActorTransform();

		// 2. 播放特效 (需要包含 Niagara 模块)
		if (TransformationVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, TransformationVFX, Location);
		}

		// 3. 生成新角色
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABaseCharacter* NewCharacter = World->SpawnActor<ABaseCharacter>(TargetCharacterClass, SpawnTransform, SpawnParams);

		if (NewCharacter)
		{
			// 4. 转移控制器 (Possess)
			AController* CurrController = GetController();
			if (CurrController)
			{
				CurrController->Possess(NewCharacter);
			}

			// 5. 销毁旧角色
			Destroy();
		}
	}
}


void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(TransformAction, ETriggerEvent::Started, this, &ABaseCharacter::TransformCharacter);
	}
}