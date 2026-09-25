#include "World/Clinic/ClinicDoor.h"

AClinicDoor::AClinicDoor()
{
    InteractionPromptKey = TEXT("ui.open");
}

void AClinicDoor::Interact_Implementation(AActor* Interactor)
{
    if (!CanInteract(Interactor))
    {
        return;
    }

    Super::Interact_Implementation(Interactor);
    if (bLocked)
    {
        PublishLocalizedText(Interactor, LockedMessageKey);
        return;
    }

    OnDoorOpened(Interactor);
}
