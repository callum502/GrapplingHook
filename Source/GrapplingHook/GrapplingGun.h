// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrapplingGun.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UGrapplingGun : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGrapplingGun();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	class AGrapplingHookCharacter* m_Character;
	class UCableComponent* m_Cable;
	class UCharacterMovementComponent* m_Movement;
	AActor* m_Attached_Object;

	void m_Left_Click();
	void m_Left_Release();
	void m_Right_Click();

	void m_Fire();
	void m_StartZip();

	void m_Update_Cable();
	void m_Swing();
	void m_Pull();
	void m_Zip();

	void m_Release();

	//variables
	float m_Pull_Force = 25000;
	float m_Zip_Force = 7000;

	FVector m_Hook_Point;
	FVector m_Out_Hit_Normal;

	FVector m_Socket_Location;
	FTimerHandle m_DelayHandle;
	float m_RotateLerp = 0;

	enum class State
	{
		none,
		zipping,
		canPull,
		pulling,
		swinging
	};
	State m_state = State::none;
};
