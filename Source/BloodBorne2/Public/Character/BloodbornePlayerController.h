#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BloodbornePlayerController.generated.h"

class ABloodbornePlayerCharacter;
class UInputMappingContext;
class UStaminaComponent;
class UTargetLockComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameplayNotification, FText, Message);

/**
 * Input and HUD bridge for the hunter. Owns the Enhanced Input mapping
 * context, installs ABloodborneCameraManager, and relays component events
 * (stamina depletion, lock-on feedback) as localized notifications that HUD
 * widgets bind to -- no widget ever polls gameplay components.
 */
UCLASS(Blueprintable)
class BLOODBORNE2_API ABloodbornePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ABloodbornePlayerController();

    virtual void BeginPlay() override;

    /** Assigned on BP_BloodbornePlayerController; added on BeginPlay. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Bloodborne|Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    /** Stacking priority for the default mapping context. */
    UPROPERTY(EditDefaultsOnly, Category="Bloodborne|Input")
    int32 MappingPriority = 0;

    /** Localized HUD feed: stamina depletion, lock-on status, gameplay hints. */
    UPROPERTY(BlueprintAssignable, Category="Bloodborne|HUD")
    FOnGameplayNotification OnGameplayNotification;

    UFUNCTION(BlueprintPure, Category="Bloodborne|Components")
    ABloodbornePlayerCharacter* GetBloodborneCharacter() const { return PossessedCharacter; }

    UFUNCTION(BlueprintPure, Category="Bloodborne|Components")
    UStaminaComponent* GetStaminaComponent() const;

    UFUNCTION(BlueprintPure, Category="Bloodborne|Components")
    UTargetLockComponent* GetTargetLockComponent() const;

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    void BindGameplayEvents(ABloodbornePlayerCharacter* InCharacter);
    void UnbindGameplayEvents();
    void BroadcastLocalizedNotification(FName LocalizationKey);
    FText ResolveLocalizedText(FName LocalizationKey) const;

    UFUNCTION()
    void HandleStaminaDepleted();

    UFUNCTION()
    void HandleTargetLocked(AActor* LockedTarget);

    UFUNCTION()
    void HandleTargetUnlocked();

    UFUNCTION()
    void HandleLockNotification(AActor* Source, FText Text);

    UPROPERTY(Transient)
    TObjectPtr<ABloodbornePlayerCharacter> PossessedCharacter;
};
