#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "MyMainMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class BLACKMYTH_WUKONG_API UMyMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
    // 初始化函数（相当于蓝图的 Construct）
    virtual void NativeConstruct() override;

    // 绑定按钮：变量名必须和UMG里的名字完全一样！
    UPROPERTY(meta = (BindWidget))
    class UButton* Btn_Start;

    UPROPERTY(meta = (BindWidget))
    class UButton* Btn_Quit;

    // 按钮点击的回调函数
    UFUNCTION()
    void OnStartClicked();

    UFUNCTION()
    void OnQuitClicked();
};
