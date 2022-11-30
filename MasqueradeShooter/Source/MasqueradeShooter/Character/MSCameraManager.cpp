
#include "MasqueradeShooter\Character\MSCameraManager.h"
#include "MasqueradeShooter/Character/MSMovementComponent.h"
#include "MasqueradeShooter/Character/MSCharacter.h"
#include "Components/CapsuleComponent.h"


AMSCameraManager::AMSCameraManager()
{
}

void AMSCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);

	if (AMSCharacter* MSCharacter = Cast<AMSCharacter>(GetOwningPlayerController()->GetPawn()))
	{
		UMSMovementComponent* MC = MSCharacter->GetMSCharacterMovement();
		FVector TargetCrouchOffset = FVector(0, 0, MC->GetCrouchedHalfHeight() - MSCharacter->GetClass()->GetDefaultObject<ACharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		FVector Offset = FMath::Lerp(FVector::ZeroVector, TargetCrouchOffset, FMath::Clamp(CrouchBlendTime / CrouchBlendDuration, 0.f, 1.f));

		if (MC->IsCrouching())
		{
			CrouchBlendTime = FMath::Clamp(CrouchBlendTime + DeltaTime, 0.f, CrouchBlendDuration);
			Offset -= TargetCrouchOffset;
		}
		else
		{
			CrouchBlendTime = FMath::Clamp(CrouchBlendTime - DeltaTime, 0.f, CrouchBlendDuration);
		}

		if (MC->IsMovingOnGround())
		{
			OutVT.POV.Location += Offset;
		}
	}
}