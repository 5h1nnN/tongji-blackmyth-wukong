// Copyright Epic Games, Inc. All Rights Reserved.

#include "blackmyth_wukongGameMode.h"
#include "blackmyth_wukongCharacter.h"
#include "UObject/ConstructorHelpers.h"

Ablackmyth_wukongGameMode::Ablackmyth_wukongGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
