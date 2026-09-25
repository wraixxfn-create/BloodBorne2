#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BloodborneTargetable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable, meta=(DisplayName="Bloodborne Lock-On Target"))
class UBloodborneTargetable : public UInterface
{
    GENERATED_BODY()
};

/**
 * Opt-in contract for lock-on candidates (Step 5 enemies, special interactables).
 * UTargetLockComponent works without it (falls back to actor location framing),
 * but implementing it lets a target tune focus height and veto locking.
 *
 * C++ implementers provide the matching _Implementation overrides, e.g.:
 *   virtual FVector GetTargetLockFocusLocation_Implementation() override;
 */
class BLOODBORNE2_API IBloodborneTargetable
{
    GENERATED_BODY()

public:
    /** World-space point the lock-on camera frames and quicksteps orbit around. */
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Targeting")
    FVector GetTargetLockFocusLocation();

    /** Gameplay veto, e.g. phased-out, dying, or otherwise invalid states. */
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Targeting")
    bool CanBeLockedOn(AActor* RequestingActor);
};
