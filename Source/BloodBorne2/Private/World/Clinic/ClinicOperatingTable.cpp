#include "World/Clinic/ClinicOperatingTable.h"
#include "Components/ArrowComponent.h"

AClinicOperatingTable::AClinicOperatingTable()
{
    InteractionPromptKey = TEXT("ui.examine");
    bInteractionEnabled = false;

    SpawnAnchor = CreateDefaultSubobject<UArrowComponent>(TEXT("PlayerSpawnAnchor"));
    SpawnAnchor->SetupAttachment(SceneRoot);
    SpawnAnchor->SetRelativeLocation(FVector(0.0, 0.0, 100.0));
    SpawnAnchor->ArrowColor = FColor(88, 160, 220);
}

FTransform AClinicOperatingTable::GetSpawnTransform() const
{
    return SpawnAnchor->GetComponentTransform();
}
