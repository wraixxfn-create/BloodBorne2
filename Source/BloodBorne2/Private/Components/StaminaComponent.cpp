#include "Components/StaminaComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogBloodborneStamina, Log, All);

UStaminaComponent::UStaminaComponent()
{
    // Regeneration is driven from TickComponent, so tick must start enabled.
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    CurrentStamina = MaxStamina;
}

void UStaminaComponent::BeginPlay()
{
    Super::BeginPlay();

    CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, MaxStamina);
    BroadcastStaminaChanged();
}

void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (DeltaTime <= 0.0f)
    {
        return;
    }

    if (TimeUntilRegeneration > 0.0f)
    {
        TimeUntilRegeneration -= DeltaTime;
        if (TimeUntilRegeneration > 0.0f)
        {
            return;
        }

        if (bExhausted)
        {
            bExhausted = false;
            UE_LOG(LogBloodborneStamina, Verbose, TEXT("%s recovered from exhaustion."),
                *GetNameSafe(GetOwner()));
        }

        if (!bRegenerationBlocked && CurrentStamina < MaxStamina)
        {
            OnStaminaRegenerationStarted.Broadcast();
        }
    }

    if (bRegenerationBlocked || CurrentStamina >= MaxStamina)
    {
        return;
    }

    ApplyStaminaChange(RegenerationSpeed * DeltaTime);
}

bool UStaminaComponent::TryConsumeStamina(float Amount)
{
    if (Amount <= 0.0f)
    {
        return true;
    }
    if (CurrentStamina < Amount)
    {
        return false;
    }

    ApplyStaminaChange(-Amount);
    TimeUntilRegeneration = FMath::Max(TimeUntilRegeneration, RegenerationDelay);
    return true;
}

void UStaminaComponent::RestoreStamina(float Amount)
{
    if (Amount <= 0.0f)
    {
        return;
    }

    ApplyStaminaChange(Amount);
}

void UStaminaComponent::DrainStaminaPerSecond(float AmountPerSecond, float DeltaTime)
{
    if (AmountPerSecond <= 0.0f || DeltaTime <= 0.0f || CurrentStamina <= SprintExhaustionFloor)
    {
        return;
    }

    const float DrainAmount = FMath::Min(AmountPerSecond * DeltaTime,
        CurrentStamina - SprintExhaustionFloor);
    ApplyStaminaChange(-DrainAmount);
    TimeUntilRegeneration = FMath::Max(TimeUntilRegeneration, RegenerationDelay);
}

void UStaminaComponent::SetRegenerationBlocked(bool bNewBlocked)
{
    bRegenerationBlocked = bNewBlocked;
}

void UStaminaComponent::SetMaxStamina(float NewMaxStamina)
{
    MaxStamina = FMath::Max(1.0f, NewMaxStamina);
    SprintExhaustionFloor = FMath::Min(SprintExhaustionFloor, MaxStamina);
    CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, MaxStamina);
    BroadcastStaminaChanged();
}

float UStaminaComponent::GetNormalizedStamina() const
{
    if (MaxStamina <= KINDA_SMALL_NUMBER)
    {
        return 0.0f;
    }
    return CurrentStamina / MaxStamina;
}

float UStaminaComponent::GetRemainingPenaltyDelay() const
{
    return bExhausted ? FMath::Max(TimeUntilRegeneration, 0.0f) : 0.0f;
}

void UStaminaComponent::ApplyStaminaChange(float Delta)
{
    if (FMath::IsNearlyZero(Delta))
    {
        return;
    }

    const float PreviousStamina = CurrentStamina;
    CurrentStamina = FMath::Clamp(CurrentStamina + Delta, 0.0f, MaxStamina);

    if (Delta < 0.0f && CurrentStamina <= 0.0f && PreviousStamina > 0.0f)
    {
        EnterExhaustion();
    }

    BroadcastStaminaChanged();
}

void UStaminaComponent::EnterExhaustion()
{
    bExhausted = true;
    TimeUntilRegeneration = FMath::Max(RegenerationDelay, ExhaustionPenaltyDelay);
    UE_LOG(LogBloodborneStamina, Log, TEXT("%s is exhausted; regeneration locked for %.2fs."),
        *GetNameSafe(GetOwner()), TimeUntilRegeneration);
    OnStaminaDepleted.Broadcast();
}

void UStaminaComponent::BroadcastStaminaChanged()
{
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, GetNormalizedStamina());
}
