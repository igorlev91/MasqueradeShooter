#include "MasqueradeShooter\Character\MSAnimInstance.h"
#include "MasqueradeShooter\Character\MSCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UMSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	 MasqueradeCharacter = Cast<AMSCharacter>(TryGetPawnOwner());
}

void UMSAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (MasqueradeCharacter == nullptr)
	{
		MasqueradeCharacter = Cast<AMSCharacter>(TryGetPawnOwner());
	}
	if (MasqueradeCharacter == nullptr) return;

	FVector Velocity = MasqueradeCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = MasqueradeCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = MasqueradeCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
}