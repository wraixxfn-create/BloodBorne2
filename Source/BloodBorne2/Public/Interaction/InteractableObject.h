#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableObject.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionText, AActor*, Interactor, FText, Text);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectInteracted, AActor*, Interactor);

/** Common event-driven contract for every world object the player can inspect or use. */
UCLASS(Abstract, Blueprintable)
class BLOODBORNE2_API AInteractableObject : public AActor
{
    GENERATED_BODY()

public:
    AInteractableObject();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
    void Interact(AActor* Interactor);
    virtual void Interact_Implementation(AActor* Interactor);

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Interaction")
    bool CanInteract(AActor* Interactor) const;
    virtual bool CanInteract_Implementation(AActor* Interactor) const;

    UFUNCTION(BlueprintPure, Category="Interaction")
    FText GetInteractionPrompt() const;

    UPROPERTY(BlueprintAssignable, Category="Interaction")
    FOnInteractionText OnInteractionText;

    UPROPERTY(BlueprintAssignable, Category="Interaction")
    FOnObjectInteracted OnInteracted;

protected:
    FText ResolveLocalizedText(FName Key) const;
    void PublishLocalizedText(AActor* Interactor, FName Key);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Interaction")
    FName InteractionPromptKey = TEXT("ui.interact");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    bool bInteractionEnabled = true;
};
