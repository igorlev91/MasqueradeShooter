#include "MasqueradeShooter/Character/MSMovementComponent.h"
#include "MasqueradeShooter/Character/MSCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

#pragma region Saved Move

UMSMovementComponent::FSavedMove_Masquerade::FSavedMove_Masquerade()
{
	Saved_bWantsToSprint = 0;
}

bool UMSMovementComponent::FSavedMove_Masquerade::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_Masquerade* NewMasqueradeMove = static_cast<FSavedMove_Masquerade*>(NewMove.Get());

	if (Saved_bWantsToSprint != NewMasqueradeMove->Saved_bWantsToSprint)
	{
		return false;
	}

	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void UMSMovementComponent::FSavedMove_Masquerade::Clear()
{
	FSavedMove_Character::Clear();

	Saved_bWantsToSprint = 0;
}

uint8 UMSMovementComponent::FSavedMove_Masquerade::GetCompressedFlags() const
{
	uint8 Result = FSavedMove_Character::GetCompressedFlags();

	if (Saved_bWantsToSprint) Result |= FLAG_Custom_0;

	return Result;
}

void UMSMovementComponent::FSavedMove_Masquerade::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	const UMSMovementComponent* CharacterMovement = Cast<UMSMovementComponent>(C->GetCharacterMovement());

	Saved_bWantsToSprint = CharacterMovement->Safe_bWantsToSprint;
}

void UMSMovementComponent::FSavedMove_Masquerade::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);

	UMSMovementComponent* CharacterMovement = Cast<UMSMovementComponent>(C->GetCharacterMovement());

	CharacterMovement->Safe_bWantsToSprint = Saved_bWantsToSprint;
}

#pragma endregion

#pragma region Client Network Prediction Data

UMSMovementComponent::FNetworkPredictionData_Client_Masquerade::FNetworkPredictionData_Client_Masquerade(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr UMSMovementComponent::FNetworkPredictionData_Client_Masquerade::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Masquerade());
}

#pragma endregion


#pragma region CMC

UMSMovementComponent::UMSMovementComponent()
{
}

void UMSMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	MSCharacterOwner = Cast<AMSCharacter>(GetOwner());
}

FNetworkPredictionData_Client* UMSMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr)

		if (ClientPredictionData == nullptr)
		{
			UMSMovementComponent* MutableThis = const_cast<UMSMovementComponent*>(this);

			MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Masquerade(*this);
			MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
			MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
		}
	return ClientPredictionData;
}

void UMSMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	Safe_bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

void UMSMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	// Check Condition for entering slide
	if (IsMovementMode(MOVE_Walking) && !bWantsToCrouch && IsCrouching())
	{
		if (Velocity.Size() > SlideMinSpeed)
		{
			EnterSlide();
			bWantsToCrouch = true;
		}
	}

	if (IsMovementMode(MOVE_Custom, CMOVE_Slide) && !bWantsToCrouch)
	{
		SetMovementMode(MOVE_Walking);
	}
}

void UMSMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if (MovementMode == MOVE_Walking)
	{
		if (Safe_bWantsToSprint)
		{
			MaxWalkSpeed = Sprint_MaxWalkSpeed;
		}
		else
		{
			MaxWalkSpeed = Walk_MaxWalkSpeed;
		}
	}
}

void UMSMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	if (GetOwner()->GetLocalRole() < ROLE_AutonomousProxy) return;

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		PhysSlide(deltaTime, Iterations);
		break;
	default:
		return;
	}
}

float UMSMovementComponent::GetMaxSpeed() const
{
	if (MovementMode != MOVE_Custom)
	{
		return Super::GetMaxSpeed();
	}
	else
	{
		switch (CustomMovementMode)
		{
		case CMOVE_Slide:
			return SlideMaxSpeed;
		default:
			return MaxCustomMovementSpeed;
		}
	}
}

#pragma endregion

#pragma region Slide

void UMSMovementComponent::EnterSlide()
{
	Velocity += UpdatedComponent->GetForwardVector() * SlideEnterImpulse;
	SetMovementMode(MOVE_Custom, CMOVE_Slide);
}

bool UMSMovementComponent::GetSlideSurface(FHitResult& SurfaceHit) const
{
	FVector Start = GetCapBottomCenter();
	FVector End = GetCapBottom() - UpdatedComponent->GetUpVector() * SlideMaxSuraceDistance;
	return GetWorld()->LineTraceSingleByProfile(SurfaceHit, Start, End, TEXT("BlockAll"), MSCharacterOwner->GetIgnoreCharacterParams());
}

void UMSMovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	RestorePreAdditiveRootMotionVelocity();

	FHitResult SurfaceHit;
	if (!GetSlideSurface(SurfaceHit))
	{
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(deltaTime, Iterations++);
		return;
	}

	if (Velocity.Size() < SlideMinSpeed)
	{
		SetMovementMode(MOVE_Walking);
		StartNewPhysics(deltaTime, Iterations++);
		return;
	}

	FVector SurfaceTangent = FVector::VectorPlaneProject(Velocity, SurfaceHit.Normal).GetSafeNormal();
	float OldTangentSpeed = FVector::VectorPlaneProject(Velocity, SurfaceHit.Normal).Size();

	// No input control, only Surface Gravity
	Acceleration = FVector(0, 0, -GetGravityZ());

	// Look-based Steering


	// Calc Velocity
	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		CalcVelocity(deltaTime, SlideFriction, true, GetMaxBrakingDeceleration());
	}
	ApplyRootMotionToVelocity(deltaTime);

	// Perform Move
	Iterations++;
	bJustTeleported = false;

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	FQuat OldRotation = UpdatedComponent->GetComponentRotation().Quaternion();
	FHitResult Hit(1.f);
	FVector Adjusted = Velocity * deltaTime;
	SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (Hit.Time < 1.f)
	{
		HandleImpact(Hit, deltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
	}

	// Update Outgoing Velocity & Acceleration
	if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}
}

#pragma endregion

#pragma region Helpers

FVector UMSMovementComponent::GetCapTop() const
{
	return UpdatedComponent->GetComponentLocation() + UpdatedComponent->GetUpVector() * CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}
FVector UMSMovementComponent::GetCapTopCenter() const
{
	return UpdatedComponent->GetComponentLocation() + UpdatedComponent->GetUpVector() * (CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius());
}
FVector UMSMovementComponent::GetCapBottom() const
{
	return UpdatedComponent->GetComponentLocation() - UpdatedComponent->GetUpVector() * CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}
FVector UMSMovementComponent::GetCapBottomCenter() const
{
	return UpdatedComponent->GetComponentLocation() - UpdatedComponent->GetUpVector() * (CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius());
}
float UMSMovementComponent::GetCapRadius() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
}
float UMSMovementComponent::GetCapHalfHeight() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

bool UMSMovementComponent::IsMovementMode(EMovementMode InMovementMode, ECustomMovementMode InCustomMovementMode) const
{
	return MovementMode == InMovementMode && CustomMovementMode == InCustomMovementMode;
}

#pragma endregion

#pragma region Input

void UMSMovementComponent::SprintPressed()
{
	Safe_bWantsToSprint = true;
}
void UMSMovementComponent::SprintReleased()
{
	Safe_bWantsToSprint = false;
}

void UMSMovementComponent::CrouchPressed()
{
	bWantsToCrouch = !bWantsToCrouch;
}

#pragma endregion