// Copyright Epic Games, Inc. All Rights Reserved.

#include "GrapplingHookGameMode.h"
#include "GrapplingHookCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGrapplingHookGameMode::AGrapplingHookGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
