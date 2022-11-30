#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MSMovementComponent.generated.h"


UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None			UMETA(Hidden),
	CMOVE_Slide			UMETA(DisplayName = "Slide"),
	CMOVE_MAX			UMETA(Hidden),
};

UCLASS()
class MASQUERADESHOOTER_API UMSMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
	class FSavedMove_Masquerade : public FSavedMove_Character
	{
		uint8 Saved_bWantsToSprint : 1;

	public:
		FSavedMove_Masquerade();

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
	};

	class FNetworkPredictionData_Client_Masquerade : public FNetworkPredictionData_Client_Character
	{
	public:
		FNetworkPredictionData_Client_Masquerade(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		virtual FSavedMovePtr AllocateNewMove() override;
	};

	// Parameters
	UPROPERTY(EditDefaultsOnly) float Sprint_MaxWalkSpeed;
	UPROPERTY(EditDefaultsOnly) float Walk_MaxWalkSpeed;
	UPROPERTY(EditDefaultsOnly) float SlideMaxSpeed = 1000;
	UPROPERTY(EditDefaultsOnly) float SlideMinSpeed = 600;
	UPROPERTY(EditDefaultsOnly) float SlideEnterImpulse = 300;
	UPROPERTY(EditDefaultsOnly) float SlideMaxSuraceDistance = 100;
	UPROPERTY(EditDefaultsOnly) float SlideFriction = .1;

	// Transient
	UPROPERTY(Transient) class AMSCharacter* MSCharacterOwner;
	bool Safe_bWantsToSprint;

public:
	UMSMovementComponent();
public:
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
protected:
	virtual void InitializeComponent() override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual float GetMaxSpeed() const override;

private:
	void EnterSlide();
	bool GetSlideSurface(FHitResult& SurfaceHit) const;
	void PhysSlide(float deltaTime, int32 Iterations);


	// Helpers
	FVector GetCapTop() const;
	FVector GetCapTopCenter() const;
	FVector GetCapBottom() const;
	FVector GetCapBottomCenter() const;
	float GetCapRadius() const;
	float GetCapHalfHeight() const;
	bool IsMovementMode(EMovementMode InMovementMode, ECustomMovementMode InCustomMovementMode = CMOVE_None) const;

public:
	UFUNCTION(BlueprintCallable) void SprintPressed();
	UFUNCTION(BlueprintCallable) void SprintReleased();

	UFUNCTION(BlueprintCallable) void CrouchPressed();
};
