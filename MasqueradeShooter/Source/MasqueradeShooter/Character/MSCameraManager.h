// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "MSCameraManager.generated.h"


UCLASS()
class MASQUERADESHOOTER_API AMSCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
private:
	UPROPERTY(EditDefaultsOnly) float CrouchBlendDuration = .5f;
	float CrouchBlendTime;


public:
	AMSCameraManager();

	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;
};
