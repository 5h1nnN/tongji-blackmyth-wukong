#include "MyMainMenuWidget.h"
#include "Kismet/GameplayStatics.h" // 用于打开关卡
#include "Kismet/KismetSystemLibrary.h" // 用于退出游戏

void UMyMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 绑定点击事件
    if (Btn_Start)
    {
        Btn_Start->OnClicked.AddDynamic(this, &UMyMainMenuWidget::OnStartClicked);
    }

    if (Btn_Quit)
    {
        Btn_Quit->OnClicked.AddDynamic(this, &UMyMainMenuWidget::OnQuitClicked);
    }
}

void UMyMainMenuWidget::OnStartClicked()
{
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("start game"));
    UGameplayStatics::OpenLevel(this, FName("MedievalVillage"));
}

void UMyMainMenuWidget::OnQuitClicked()
{
    // 退出游戏
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}