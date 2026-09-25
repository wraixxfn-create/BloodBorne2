#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractableObject.h"
#include "ClinicDoor.generated.h"

/** Locked sickroom door. Animation remains Blueprint-extensible through OnDoorOpened. */
UCLASS(Blueprintable)
class BLOODBORNE2_API AClinicDoor : public AInteractableObject
{
    GENERATED_BODY()

public:
    AClinicDoor();
    virtual void Interact_Implementation(AActor* Interactor) override;

    UFUNCTION(BlueprintCallable, Category="Clinic Door")
    void SetLocked(bool bNewLocked) { bLocked = bNewLocked; }

    UFUNCTION(BlueprintPure, Category="Clinic Door")
    bool IsLocked() const { return bLocked; }

protected:
    UFUNCTION(BlueprintImplementableEvent, Category="Clinic Door")
    void OnDoorOpened(AActor* Interactor);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clinic Door")
    bool bLocked = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Clinic Door")
    FName LockedMessageKey = TEXT("interactables.clinic_door.locked");
};
