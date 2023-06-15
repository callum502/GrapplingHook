// Fill out your copyright notice in the Description page of Project Settings.


#include "GrapplingGun.h"

// Fill out your copyright notice in the Description page of Project Settings.


#include "CableComponent.h"

#include "Math/Vector.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GrapplingHookCharacter.h"
#include "Components/InputComponent.h"

// Sets default values for this component's properties
UGrapplingGun::UGrapplingGun()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Called when the game starts
void UGrapplingGun::BeginPlay()
{
	//get parent
	AActor* Owner = GetOwner();

	//set Character variables
	m_Character = Cast<AGrapplingHookCharacter>(Owner);
	m_Cable = m_Character->m_Cable;
	m_Movement = m_Character->GetCharacterMovement();

	//setup input
	m_Character->InputComponent->BindAction("Leftclick", IE_Pressed, this, &UGrapplingGun::m_Left_Click);
	m_Character->InputComponent->BindAction("Leftclick", IE_Released, this, &UGrapplingGun::m_Left_Release);
	m_Character->InputComponent->BindAction("RightClick", IE_Pressed, this, &UGrapplingGun::m_Right_Click);

	//hide cable
	m_Cable->SetHiddenInGame(true);

	//default params
	m_Movement->AirControl = 1;
	m_Cable->CableWidth = 6.f;
	m_Cable->NumSides = 6;
}

void UGrapplingGun::m_Left_Click()
{
	if (m_state == State::canPull)
		m_state = State::pulling;
	else if (m_state == State::none)
	{
		m_state = State::swinging;
		m_Fire();
	}
}

void UGrapplingGun::m_Left_Release()
{
	if (m_state == State::swinging)
		m_Release();
	else if (m_state == State::pulling)
		m_state = State::canPull;
}

void UGrapplingGun::m_Right_Click()
{
	if (m_state == State::pulling || m_state == State::canPull)
	{
		m_Release();
	}
	else if (m_state == State::none)
	{
		m_Fire();
	}
}

// Called every frame
void UGrapplingGun::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (m_state != State::none)
		m_Update_Cable();

	//state case
	switch (m_state)
	{
	case UGrapplingGun::State::zipping:
		m_Zip();
		break;
	case UGrapplingGun::State::pulling:
		m_Pull();
		break;
	case UGrapplingGun::State::swinging:
		m_Swing();
		break;
	default:
		break;
	}
}

void UGrapplingGun::m_Update_Cable()
{
	m_Cable->SetHiddenInGame(false);
	FVector direction = m_Character->GetActorLocation() - m_Hook_Point;
	m_Cable->CableLength = direction.Size() * 0.1;
	m_Cable->EndLocation = m_Socket_Location;
	if (m_state == State::pulling || m_state == State::canPull)
		m_Hook_Point = Cast<UStaticMeshComponent>(m_Attached_Object->GetComponentByClass(UStaticMeshComponent::StaticClass()))->GetComponentLocation();
	m_Cable->SetWorldLocation(m_Hook_Point);
}

void UGrapplingGun::m_Fire()
{
	//get player controller
	APlayerController* PlayerController = Cast<APlayerController>(m_Character->GetController());
	FVector m_Out_Hit_Location;
	FVector m_Out_Hit_Direction;

	//get screen size
	int x, y;
	PlayerController->GetViewportSize(x, y);
	PlayerController->DeprojectScreenPositionToWorld(x / 2, y / 2, m_Out_Hit_Location, m_Out_Hit_Direction);
	FVector startPoint = m_Out_Hit_Location;
	FVector endpoint = (m_Out_Hit_Location + m_Out_Hit_Direction * 10000.0);
	TArray<AActor*> actorsToIgnore;
	actorsToIgnore.Add(m_Character);
	FHitResult OutHit;

	if (UKismetSystemLibrary::LineTraceSingle(this, startPoint, endpoint, UEngineTypes::ConvertToTraceType(ECC_Visibility), false, actorsToIgnore, EDrawDebugTrace::None, OutHit, true))
	{
		//update outhit variables
		m_Hook_Point = OutHit.Location;
		m_Out_Hit_Normal = OutHit.Normal;
		m_Attached_Object = OutHit.GetActor();

		//select hand to grapple with
		//Calculate the angle between the hit normal and the actor's right vector
		float angle = FMath::Acos(FVector::DotProduct(m_Out_Hit_Normal, m_Character->GetActorRightVector()));
		//Convert angle from radians to degrees
		float angleDegrees = FMath::RadiansToDegrees(angle);
		if (angleDegrees > 90 && angleDegrees < 270)
		{
			// Hit point is to the right of the player
			m_Socket_Location = m_Character->GetMesh()->GetSocketTransform("hand_rSocket", RTS_Actor).GetLocation();
		}
		else
		{
			// Hit point is to the left of the player
			m_Socket_Location = m_Character->GetMesh()->GetSocketTransform("hand_lSocket", RTS_Actor).GetLocation();
		}

		//update state
		if (m_state != State::swinging)
		{
			if (OutHit.GetActor()->ActorHasTag("Pullable"))
			{
				m_state = State::canPull;
			}
			else if (m_state == State::none)
			{
				if (!m_Movement->IsFalling())
				{
					m_Movement->AddImpulse(FVector(0, 0, 100000));
					GetWorld()->GetTimerManager().SetTimer(m_DelayHandle, this, &UGrapplingGun::m_StartZip, 0.6, false);
				}
				else
					m_StartZip();
			}
		}
	}
	else
		m_state = State::none; //if fire misses
}

void UGrapplingGun::m_StartZip()
{
	m_Movement->Velocity = FVector(0, 0, 0);
	m_Movement->GravityScale = 0;
	m_Character->JumpMaxCount = 0;
	m_Movement->GroundFriction = 0;
	m_state = State::zipping;
}

void UGrapplingGun::m_Swing()
{
	if (m_Hook_Point.Z > m_Character->GetActorLocation().Z && m_Movement->IsFalling())
	{
		//get dot product of player velocity and player to hook point
		float dot = UKismetMathLibrary::Dot_VectorVector(UKismetMathLibrary::Normal(m_Character->GetVelocity()), UKismetMathLibrary::Normal(m_Hook_Point - m_Character->GetActorLocation()));
		//force to swing playern in arc
		FVector SwingForce = -dot * 200000 * UKismetMathLibrary::Normal(m_Hook_Point - m_Character->GetActorLocation());
		m_Movement->AddImpulse(SwingForce);

		//force to push player from wall
		FVector wallForce = m_Out_Hit_Normal * 2000000 * (1 / (m_Hook_Point - m_Character->GetActorLocation()).Size());
		m_Movement->AddImpulse(wallForce);
	}
	else
		m_Release();
}

void UGrapplingGun::m_Zip()
{
	//lerp rotate toward target
	FRotator rot = UKismetMathLibrary::FindLookAtRotation(m_Character->GetActorLocation(), m_Hook_Point);
	//lerp towards rotation
	rot = UKismetMathLibrary::RLerp(m_Character->GetActorRotation(), rot, m_RotateLerp, true);
	if (m_RotateLerp < 1)
		m_RotateLerp += 0.01;
	m_Character->SetActorRotation(FRotator(0, rot.Yaw, 0));

	//apply force
	FVector force = UKismetMathLibrary::Normal(m_Hook_Point - m_Character->GetActorLocation()) * m_Zip_Force;
	m_Movement->AddImpulse(force);

	//if player is close to hook point then release
	if (UKismetMathLibrary::Vector_GetAbsMax(m_Character->GetActorLocation() - m_Hook_Point) < 100)
		m_Release();
}

void UGrapplingGun::m_Pull()
{
	FVector force;
	force = UKismetMathLibrary::Normal(m_Character->GetActorLocation() - m_Hook_Point) * m_Pull_Force;
	//get mesh of attached object and apply force
	Cast<UStaticMeshComponent>(m_Attached_Object->GetComponentByClass(UStaticMeshComponent::StaticClass()))->AddImpulse(force);
}

void UGrapplingGun::m_Release()
{
	//apply release forces and reset character variables
	switch (m_state)
	{
	case State::zipping:
		m_Movement->GravityScale = 1;
		m_Movement->GroundFriction = 8;
		m_Movement->Velocity = FVector(0, 0, 0);
		m_Character->JumpMaxCount = 1;

		//launch if hitpoint is wall
		if (0.9 > m_Out_Hit_Normal.Z)
			m_Character->LaunchCharacter(FVector(0, 0, 1000), false, false);
		break;
	case State::swinging:
		//apply force based on player velocity //magnitude capped at 10000000
		m_Movement->AddImpulse(FMath::Min(200000, m_Character->GetVelocity().Size() * 200) * UKismetMathLibrary::Normal(m_Character->GetVelocity()));
		break;
	default:
		break;
	}

	//reset state and cable
	m_state = State::none;
	m_Cable->SetHiddenInGame(true);
	m_Cable->EndLocation = FVector(0, 0, 0);
}