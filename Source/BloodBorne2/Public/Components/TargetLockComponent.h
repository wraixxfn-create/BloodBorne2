#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Core/BloodborneTypes.h"
#include "TargetLockComponent.generated.h"

class AActor;
class USphereComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetLocked, AActor*, LockedTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTargetUnlocked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockedTargetChanged, AActor*, NewTarget);

/**
 * Decoupled lock-on service: candidate acquisition (sphere scan + cone + line
 * of sight), target scoring, cycling, and validity upkeep. The camera frames
 * the target and the character faces it; this component only answers "who is
 * locked" and "where do I aim", so camera/animation/HUD layers bind to its
 * delegates instead of polling.
 *
 * Candidates are actors whose collision overlaps the BBTargeting trace channel
 * (opt-in for tuned targets) or any pawn inside the detection sphere. Implement
 * IBloodborneTargetable to control focus height or veto locking entirely.
 */
UCLASS(ClassGroup=(Bloodborne), meta=(BlueprintSpawnableComponent))
class BLOODBORNE2_API UTargetLockComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTargetLockComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    /**
     * Input entry point: toggles lock-on. When no candidate is available the
     * toggle is refused and a localized hint is emitted (hud.lock_on_no_target).
     */
    UFUNCTION(BlueprintCallable, Category="Targeting")
    void ToggleLockOn();

    /** Cycles to the nearest candidate on the given side (+1 right, -1 left). */
    UFUNCTION(BlueprintCallable, Category="Targeting")
    void CycleTarget(int32 Direction);

    /** External veto: combat death, cutscenes, menus. Force unlocks when active. */
    UFUNCTION(BlueprintCallable, Category="Targeting")
    void SetLockEnabled(bool bNewEnabled);

    /** Hard unlock (target died, left the arena, lock disabled). */
    UFUNCTION(BlueprintCallable, Category="Targeting")
    void ClearLockedTarget();

    UFUNCTION(BlueprintPure, Category="Targeting")
    AActor* GetLockedTarget() const { return LockedTarget; }

    UFUNCTION(BlueprintPure, Category="Targeting")
    bool HasLockedTarget() const { return IsValid(LockedTarget); }

    UFUNCTION(BlueprintPure, Category="Targeting")
    bool IsLockOnActive() const { return bLockOnActive; }

    /** Focus point used by the camera and HUD to frame the target. */
    UFUNCTION(BlueprintPure, Category="Targeting")
    FVector GetLockedFocusLocation() const;

    /** Cached visibility for HUD reticle fading (refreshed each validity check). */
    UFUNCTION(BlueprintPure, Category="Targeting")
    bool IsLockedTargetVisible() const { return bLockedTargetVisible; }

    /** Fires after a successful lock, carrying the newly locked target. */
    UPROPERTY(BlueprintAssignable, Category="Targeting")
    FOnTargetLocked OnTargetLocked;

    /** Fires when lock-on is released or the last candidate disappears. */
    UPROPERTY(BlueprintAssignable, Category="Targeting")
    FOnTargetUnlocked OnTargetUnlocked;

    /** Fires for every locked target change (first lock, cycle, clear). */
    UPROPERTY(BlueprintAssignable, Category="Targeting")
    FOnLockedTargetChanged OnLockedTargetChanged;

    /**
     * Localized feedback channel: hud.lock_on_no_target / hud.lock_on_lost_target.
     * HUD binds here; the component never touches widgets directly.
     */
    UPROPERTY(BlueprintAssignable, Category="Targeting")
    FOnLocalizedNotification OnLockNotification;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void AttachDetectionVolume();
    void UpdateLockValidity();
    void UpdateLockedTargetVisibility();
    bool TryAcquireInitialTarget();
    void GatherCandidates(TArray<AActor*>& OutCandidates, bool bRequireLineOfSight) const;
    bool IsCandidateValid(AActor* Candidate) const;
    bool HasLineOfSightTo(AActor* Candidate) const;
    float ScoreCandidate(AActor* Candidate) const;
    void LockActor(AActor* Candidate);
    void PublishLockNotification(FName LocalizationKey);

    /** World-space focus point of any candidate (interface-aware fallback). */
    FVector ResolveFocusLocation(AActor* Target) const;

    /** Collision shape used to gather lock-on candidates. Attached in BeginPlay. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Targeting")
    TObjectPtr<USphereComponent> DetectionSphere;

    /** Actors within this radius are eligible for lock-on. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Targeting",
        meta=(ClampMin="100.0", UIMin="100.0"))
    float DetectionRadius = 2000.0f;

    /** Vertical filter so targets far above/below are ignored. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Targeting",
        meta=(ClampMin="0.0"))
    float MaxHeightDifference = 450.0f;

    /** Visibility/validity traces are capped even inside the detection radius. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Targeting",
        meta=(ClampMin="100.0"))
    float MaxLineOfSightDistance = 3000.0f;

    /** Candidates outside this camera-relative cone are not picked up. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Targeting",
        meta=(ClampMin="0.0", ClampMax="180.0"))
    float AcquisitionHalfAngleDeg = 75.0f;

    /** Re-check cadence for LOS/validity upkeep and reticle fading. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Targeting",
        meta=(ClampMin="0.01"))
    float ValidityCheckInterval = 0.1f;

    /** Ignore owners of this class when scanning (friendly NPCs, ...). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Targeting")
    TSubclassOf<AActor> IgnoredActorClass;

    /** Channel used for LOS traces; world geometry should block it. */
    UPROPERTY(EditAnywhere, Category="Targeting")
    TEnumAsByte<ECollisionChannel> VisibilityTraceChannel = ECC_BB_TARGETING;

    UPROPERTY(Transient)
    TObjectPtr<AActor> LockedTarget;

private:
    bool bLockOnActive = false;
    bool bLockEnabled = true;
    bool bLockedTargetVisible = false;
    float ValidityTimer = 0.0f;
};
