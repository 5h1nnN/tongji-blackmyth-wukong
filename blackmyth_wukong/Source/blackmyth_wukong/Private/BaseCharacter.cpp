#include "FeyCharacter.h"
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
	// 如果正在冷却，不允许变身
	if (bIsCooldown)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("技能冷却中..."));
		return;
	}

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
			NewCharacter->CurrentHealth = this->CurrentHealth; // 或者设置为某个固定值
			// 如果是从 Fey 变回 Wukong (即当前是 Fey)
			// 我们让新生成的 Wukong 进入冷却
			AFeyCharacter* IsFey = Cast<AFeyCharacter>(this);
			if (IsFey)
			{
				NewCharacter->StartTransformCooldown();
			}
			// 4. 转移控制器 (Possess)
			AController* CurrController = GetController();
			APlayerController* PC = Cast<APlayerController>(CurrController);
			if (PC)
			{
				// 让相机平滑混合 0.2 秒
				PC->SetViewTargetWithBlend(NewCharacter, 0.2f);
			}
			CurrController->Possess(NewCharacter);
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

void ABaseCharacter::StartTransformCooldown()
{
	bIsCooldown = true;

	GetWorldTimerManager().SetTimer(
		CooldownTimerHandle,
		this,
		&ABaseCharacter::OnCooldownFinished,
		TransformCooldownDuration,
		false
	);
}

void ABaseCharacter::OnCooldownFinished()
{
	bIsCooldown = false;
}

float ABaseCharacter::GetCooldownPercent() const
{
	if (bIsCooldown && GetWorldTimerManager().IsTimerActive(CooldownTimerHandle))
	{
		return 1.0f - FMath::Clamp(GetWorldTimerManager().GetTimerRemaining(CooldownTimerHandle) / TransformCooldownDuration, 0.0f, 1.0f);
	}
	return 1.0f;
}