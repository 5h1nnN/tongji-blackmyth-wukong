#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TitleGameMode.generated.h"

UCLASS()
class BLACKMYTH_WUKONG_API ATitleGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:
    virtual void BeginPlay() override;

    // ±©Â¶¸øÀ¶Í¼
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserWidget> MainMenuWidgetClass;
};
