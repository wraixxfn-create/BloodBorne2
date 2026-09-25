#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStaminaChanged, float, CurrentStamina, float, MaxStamina, float, NormalizedStamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaDepleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaRegenerationStarted);

/**
 * Stat-like stamina resource shared by sprinting, dodges, and heavy actions.
 * Emits events instead of owning visuals so HUD and animation layers stay
 * decoupled. Exhaustion applies a regeneration lockout (the "penalty delay")
 * before regeneration resumes.
 */
UCLASS(ClassGroup=(Bloodborne), meta=(BlueprintSpawnableComponent))
class BLOODBORNE2_API UStaminaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStaminaComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    /**
     * Attempts to spend stamina. Returns true and applies the cost, including
     * exhaustion lockout when the pool hits zero. Requests larger than the
     * available pool are refused so gameplay code decides what is mandatory.
     */
    UFUNCTION(BlueprintCallable, Category="Stamina")
    bool TryConsumeStamina(float Amount);

    /** Grants stamina immediately (items, rally regain), clamped to MaxStamina. */
    UFUNCTION(BlueprintCallable, Category="Stamina")
    void RestoreStamina(float Amount);

    /** Sprint tick drain with a hard floor at the exhaustion threshold. */
    UFUNCTION(BlueprintCallable, Category="Stamina")
    void DrainStaminaPerSecond(float AmountPerSecond, float DeltaTime);

    /** Toggles continuous regeneration suppression (aiming, staggered, ...). */
    UFUNCTION(BlueprintCallable, Category="Stamina")
    void SetRegenerationBlocked(bool bNewBlocked);

    UFUNCTION(BlueprintCallable, Category="Stamina")
    void SetMaxStamina(float NewMaxStamina);

    UFUNCTION(BlueprintPure, Category="Stamina")
    float GetCurrentStamina() const { return CurrentStamina; }

    UFUNCTION(BlueprintPure, Category="Stamina")
    float GetMaxStamina() const { return MaxStamina; }

    UFUNCTION(BlueprintPure, Category="Stamina")
    float GetNormalizedStamina() const;

    UFUNCTION(BlueprintPure, Category="Stamina")
    bool IsExhausted() const { return bExhausted; }

    UFUNCTION(BlueprintPure, Category="Stamina")
    bool HasStamina(float Amount) const { return CurrentStamina >= Amount; }

    /** Sprint drain stops at this pool slice (soft-lock prevention). */
    UFUNCTION(BlueprintPure, Category="Stamina")
    float GetSprintExhaustionFloor() const { return SprintExhaustionFloor; }

    /** Seconds remaining before regeneration resumes after exhaustion. */
    UFUNCTION(BlueprintPure, Category="Stamina")
    float GetRemainingPenaltyDelay() const;

    UPROPERTY(BlueprintAssignable, Category="Stamina")
    FOnStaminaChanged OnStaminaChanged;

    /** Fired once when stamina reaches the exhaustion threshold. */
    UPROPERTY(BlueprintAssignable, Category="Stamina")
    FOnStaminaDepleted OnStaminaDepleted;

    /** Fired when regeneration actually resumes (after any penalty delay). */
    UPROPERTY(BlueprintAssignable, Category="Stamina")
    FOnStaminaRegenerationStarted OnStaminaRegenerationStarted;

protected:
    virtual void BeginPlay() override;

    void ApplyStaminaChange(float Delta);
    void EnterExhaustion();
    void BroadcastStaminaChanged();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stamina",
        meta=(ClampMin="1.0", UIMin="1.0"))
    float MaxStamina = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stamina",
        meta=(ClampMin="0.0"))
    float RegenerationSpeed = 22.0f;

    /** Idle time before normal regeneration starts after any spend. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stamina",
        meta=(ClampMin="0.0"))
    float RegenerationDelay = 0.8f;

    /** Extra lockout applied when stamina is fully depleted. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stamina",
        meta=(ClampMin="0.0"))
    float ExhaustionPenaltyDelay = 1.5f;

    /** Sprinting never spends below this pool slice (prevents soft-locks). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stamina",
        meta=(ClampMin="0.0", UIMax="20.0"))
    float SprintExhaustionFloor = 5.0f;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stamina", meta=(AllowPrivateAccess="true"))
    float CurrentStamina = 120.0f;

    bool bExhausted = false;
    bool bRegenerationBlocked = false;
    float TimeUntilRegeneration = 0.0f;
};
