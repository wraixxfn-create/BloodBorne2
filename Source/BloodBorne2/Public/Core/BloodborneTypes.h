#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "BloodborneTypes.generated.h"

/**
 * High level locomotion/combat pose consumed by the Step 5 animation state
 * machine. Keep the numeric values stable; blueprints bind to them by value.
 */
UENUM(BlueprintType)
enum class EBloodborneMovementState : uint8
{
    /** Idle or moving on foot; camera-free or locked-on facing. */
    Normal      UMETA(DisplayName = "Normal"),

    /** Unlocked evasive roll with a fixed duration and i-frames. */
    Rolling     UMETA(DisplayName = "Rolling"),

    /** Locked-on quickstep/dash relative to the locked target. */
    Dashing     UMETA(DisplayName = "Dashing"),

    /** Interrupted/attacking window; movement input is ignored. */
    Stunned     UMETA(DisplayName = "Stunned"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovementStateChanged, EBloodborneMovementState, NewState);

/** Localized notification channel; HUD subscribes, emitters stay decoupled. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLocalizedNotification, AActor*, Source, FText, Text);

/** Channels reserved by Step 2 traces; mapped in DefaultEngine.ini. */
#define ECC_BB_TARGETING ECC_GameTraceChannel1
#define ECC_BB_INTERACTION ECC_GameTraceChannel2

/** Objects implementing this contract are eligible lock-on candidates. */
UINTERFACE(MinimalAPI, Blueprintable, meta=(DisplayName="Bloodborne Lock-On Target"))
class UBloodborneTargetable : public UInterface
{
    GENERATED_BODY()
};

class IBloodborneTargetable
{
    GENERATED_BODY()

public:
    /** World-space point the lock-on camera frames and quicksteps orbit around. */
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Targeting")
    FVector GetTargetLockFocusLocation() const;

    /** Gameplay veto, e.g. phased-out, dying, or off-screen boss states. */
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Targeting")
    bool CanBeLockedOn(AActor* RequestingActor) const;
};
