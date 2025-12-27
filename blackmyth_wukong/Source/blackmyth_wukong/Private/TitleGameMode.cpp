#include "TitleGameMode.h"
#include "Blueprint/UserWidget.h"

void ATitleGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (MainMenuWidgetClass)
    {
        // 创建 UI
        UUserWidget* MenuWidget = CreateWidget<UUserWidget>(GetWorld(), MainMenuWidgetClass);
        if (MenuWidget)
        {
            MenuWidget->AddToViewport();
        }

        // 显示鼠标
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            PC->bShowMouseCursor = true;
            PC->SetInputMode(FInputModeUIOnly());
        }
    }
}