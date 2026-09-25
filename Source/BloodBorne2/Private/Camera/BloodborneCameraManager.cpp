#include "Camera/BloodborneCameraManager.h"

#include "Character/BloodbornePlayerCharacter.h"
#include "Components/TargetLockComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

ABloodborneCameraManager::ABloodborneCameraManager()
{
    // Bloodborne-style view cone: generous downward play, limited sky.
    ViewPitchMin = -55.0f;
    ViewPitchMax = 40.0f;
    ViewYawMin = 0.0f;
    ViewYawMax = 0.0f;
    ViewRollMin = 0.0f;
    ViewRollMax = 0.0f;
    DefaultFOV = 80.0f;
}

void ABloodborneCameraManager::UpdateCamera(float DeltaTime)
{
    EnsureCameraCollisionProfile();
    ApplyLockOnFraming(DeltaTime);
    Super::UpdateCamera(DeltaTime);
}

ABloodbornePlayerCharacter* ABloodborneCameraManager::GetBloodborneCharacter() const
{
    const APlayerController* PlayerController = GetOwningPlayerController();
    if (!PlayerController)
    {
        return nullptr;
    }
    return Cast<ABloodbornePlayerCharacter>(PlayerController->GetPawn());
}

void ABloodborneCameraManager::EnsureCameraCollisionProfile()
{
    if (bCollisionProfileApplied)
    {
        return;
    }

    ABloodbornePlayerCharacter* Character = GetBloodborneCharacter();
    USpringArmComponent* SpringArm = Character ? Character->GetCameraSpringArm() : nullptr;
    if (!SpringArm)
    {
        return;
    }

    // Wall-clip protection: the arm traces on the Camera channel and pulls the
    // camera inside blocking geometry instead of letting it pass through walls.
    SpringArm->bDoCollisionTest = true;
    SpringArm->ProbeChannel = ECC_Camera;
    SpringArm->ProbeSize = CameraProbeSize;
    bCollisionProfileApplied = true;
}

void ABloodborneCameraManager::ApplyLockOnFraming(float DeltaTime)
{
    ABloodbornePlayerCharacter* Character = GetBloodborneCharacter();
    if (!Character)
    {
        return;
    }

    USpringArmComponent* SpringArm = Character->GetCameraSpringArm();
    UTargetLockComponent* TargetLock = Character->GetTargetLockComponent();
    APlayerController* PlayerController = GetOwningPlayerController();
    if (!SpringArm || !TargetLock || !PlayerController)
    {
        return;
    }

    if (CachedFreeArmLength <= 0.0f)
    {
        CachedFreeArmLength = SpringArm->TargetArmLength;
    }

    if (TargetLock->HasLockedTarget())
    {
        const FVector TargetLocation = TargetLock->GetLockedFocusLocation();
        const FVector PlayerLocation = Character->GetActorLocation()
            + FVector::UpVector * PlayerFocusHeightOffset;
        const FVector ToTarget = TargetLocation - PlayerLocation;
        const float DistanceToTarget = ToTarget.Size();

        // Keep the camera centered on the player-target axis.
        FRotator DesiredRotation = ToTarget.Rotation();
        DesiredRotation.Pitch = LockedCameraPitch;
        DesiredRotation.Roll = 0.0f;

        const FRotator CurrentRotation = PlayerController->GetControlRotation();
        PlayerController->SetControlRotation(
            FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, LockOnInterpSpeed));

        const float DesiredArmLength = FMath::Clamp(DistanceToTarget * LockedArmLengthRatio,
            MinLockedArmLength, MaxLockedArmLength);
        SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength,
            DesiredArmLength, DeltaTime, LockOnInterpSpeed);
        return;
    }

    if (FMath::Abs(SpringArm->TargetArmLength - CachedFreeArmLength) > KINDA_SMALL_NUMBER)
    {
        SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength,
            CachedFreeArmLength, DeltaTime, LockOnInterpSpeed);
    }
}
