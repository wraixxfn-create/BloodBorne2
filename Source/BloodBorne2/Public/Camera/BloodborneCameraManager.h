#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "BloodborneCameraManager.generated.h"

class ABloodbornePlayerCharacter;
class USpringArmComponent;
class UTargetLockComponent;

/**
 * Project camera manager (Bloodborne-style third person view).
 * Responsibilities:
 *  - Camera collision filtering: ensures the character's spring arm performs
 *    wall traces on the Camera channel so geometry never clips into the view.
 *  - Lock-on framing: while UTargetLockComponent reports a locked target, the
 *    control rotation is interpolated so the camera stays centered on the
 *    player-target axis and the arm length frames both actors.
 *  - Free mode keeps full control-rotation camera driven by look input.
 */
UCLASS()
class BLOODBORNE2_API ABloodborneCameraManager : public APlayerCameraManager
{
    GENERATED_BODY()

public:
    ABloodborneCameraManager();

    virtual void UpdateCamera(float DeltaTime) override;

    /** Radius of the spring arm wall trace (smaller = tighter wall hugs). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne Camera",
        meta=(ClampMin="1.0"))
    float CameraProbeSize = 12.0f;

    /** Pitch applied while framing a locked target. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne Camera",
        meta=(ClampMin="-45.0", ClampMax="45.0"))
    float LockedCameraPitch = -12.0f;

    /** How fast rotation/arm length converge onto the lock-on framing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne Camera",
        meta=(ClampMin="0.1"))
    float LockOnInterpSpeed = 8.0f;

    /** Arm length = distance to target * ratio, clamped below. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne Camera",
        meta=(ClampMin="0.1", ClampMax="1.0"))
    float LockedArmLengthRatio = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne Camera",
        meta=(ClampMin="100.0"))
    float MinLockedArmLength = 220.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne Camera",
        meta=(ClampMin="150.0"))
    float MaxLockedArmLength = 720.0f;

    /** Height added to the player pivot when framing the midpoint. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne Camera",
        meta=(ClampMin="0.0"))
    float PlayerFocusHeightOffset = 100.0f;

protected:
    ABloodbornePlayerCharacter* GetBloodborneCharacter() const;
    void EnsureCameraCollisionProfile();
    void ApplyLockOnFraming(float DeltaTime);

    bool bCollisionProfileApplied = false;
    float CachedFreeArmLength = 0.0f;
};
