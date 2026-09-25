#include "World/Clinic/ClinicNote.h"

AClinicNote::AClinicNote()
{
    InteractionPromptKey = TEXT("ui.examine");
}

void AClinicNote::Interact_Implementation(AActor* Interactor)
{
    if (!CanInteract(Interactor))
    {
        return;
    }
    Super::Interact_Implementation(Interactor);
    PublishLocalizedText(Interactor, NoteTextKey);
}
