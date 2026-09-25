#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractableObject.h"
#include "ClinicOperatingTable.generated.h"

class UArrowComponent;

/** Opening operating table and stable anchor for player spawning/cinematics. */
UCLASS(Blueprintable)
class BLOODBORNE2_API AClinicOperatingTable : public AInteractableObject
{
    GENERATED_BODY()

public:
    AClinicOperatingTable();

    UFUNCTION(BlueprintPure, Category="Clinic|Spawn")
    FTransform GetSpawnTransform() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Spawn")
    TObjectPtr<UArrowComponent> SpawnAnchor;
};
