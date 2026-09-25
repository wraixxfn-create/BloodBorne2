#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractableObject.h"
#include "ClinicNote.generated.h"

UCLASS(Blueprintable)
class BLOODBORNE2_API AClinicNote : public AInteractableObject
{
    GENERATED_BODY()

public:
    AClinicNote();
    virtual void Interact_Implementation(AActor* Interactor) override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Clinic Note")
    FName NoteTextKey = TEXT("notes.paleblood_scribble.body");
};
