#include "Interaction/InteractableObject.h"

#include "Components/StaticMeshComponent.h"
#include "Core/LocalizationManager.h"
#include "Engine/GameInstance.h"

AInteractableObject::AInteractableObject()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);
    Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void AInteractableObject::Interact_Implementation(AActor* Interactor)
{
    if (CanInteract(Interactor))
    {
        OnInteracted.Broadcast(Interactor);
    }
}

bool AInteractableObject::CanInteract_Implementation(AActor* Interactor) const
{
    return bInteractionEnabled && IsValid(Interactor);
}

FText AInteractableObject::GetInteractionPrompt() const
{
    return ResolveLocalizedText(InteractionPromptKey);
}

FText AInteractableObject::ResolveLocalizedText(const FName Key) const
{
    if (const UGameInstance* GameInstance = GetGameInstance())
    {
        if (const ULocalizationManager* Localization = GameInstance->GetSubsystem<ULocalizationManager>())
        {
            return Localization->GetLocalizedText(Key);
        }
    }
    return FText::FromName(Key);
}

void AInteractableObject::PublishLocalizedText(AActor* Interactor, const FName Key)
{
    OnInteractionText.Broadcast(Interactor, ResolveLocalizedText(Key));
}
