#include "World/Clinic/StartingClinicRoom.h"

#include "Components/ChildActorComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "World/Clinic/ClinicDoor.h"
#include "World/Clinic/ClinicNote.h"
#include "World/Clinic/ClinicOperatingTable.h"

AStartingClinicRoom::AStartingClinicRoom()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    RoomShell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoomShell_CrackedStone"));
    RoomShell->SetupAttachment(SceneRoot);
    RoomShell->SetCollisionProfileName(TEXT("BlockAll"));

    ArchedWindows = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TallArchedWindows"));
    ArchedWindows->SetupAttachment(SceneRoot);
    ArchedWindows->SetCollisionProfileName(TEXT("BlockAll"));

    Dressing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VictorianRoomDressing"));
    Dressing->SetupAttachment(SceneRoot);
    Dressing->SetCollisionProfileName(TEXT("BlockAll"));

    OperatingTable = CreateDefaultSubobject<UChildActorComponent>(TEXT("WornOperatingTable"));
    OperatingTable->SetupAttachment(SceneRoot);
    OperatingTable->SetChildActorClass(AClinicOperatingTable::StaticClass());
    OperatingTable->SetRelativeLocation(FVector(0.0, 0.0, 0.0));

    LockedDoor = CreateDefaultSubobject<UChildActorComponent>(TEXT("LockedWoodenDoor"));
    LockedDoor->SetupAttachment(SceneRoot);
    LockedDoor->SetChildActorClass(AClinicDoor::StaticClass());
    LockedDoor->SetRelativeLocation(FVector(520.0, 0.0, 0.0));

    FloorNote = CreateDefaultSubobject<UChildActorComponent>(TEXT("PalebloodFloorNote"));
    FloorNote->SetupAttachment(SceneRoot);
    FloorNote->SetChildActorClass(AClinicNote::StaticClass());
    FloorNote->SetRelativeLocation(FVector(175.0, -110.0, 2.0));

    Moonlight = CreateDefaultSubobject<URectLightComponent>(TEXT("CoolWindowMoonlight"));
    Moonlight->SetupAttachment(SceneRoot);
    Moonlight->SetRelativeLocation(FVector(-350.0, 0.0, 320.0));
    Moonlight->SetRelativeRotation(FRotator(-25.0, 0.0, 0.0));
    Moonlight->SourceWidth = 220.0f;
    Moonlight->SourceHeight = 420.0f;
    Moonlight->AttenuationRadius = 1400.0f;

    CandleLightA = CreateDefaultSubobject<UPointLightComponent>(TEXT("CandleLightA"));
    CandleLightA->SetupAttachment(SceneRoot);
    CandleLightA->SetRelativeLocation(FVector(80.0, 210.0, 125.0));
    CandleLightA->AttenuationRadius = 430.0f;
    CandleLightA->bUseInverseSquaredFalloff = true;

    CandleLightB = CreateDefaultSubobject<UPointLightComponent>(TEXT("CandleLightB"));
    CandleLightB->SetupAttachment(SceneRoot);
    CandleLightB->SetRelativeLocation(FVector(390.0, -180.0, 105.0));
    CandleLightB->AttenuationRadius = 360.0f;
    CandleLightB->bUseInverseSquaredFalloff = true;

    ApplyLightingSettings();
}

void AStartingClinicRoom::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyLightingSettings();
}

FTransform AStartingClinicRoom::GetPlayerSpawnTransform() const
{
    if (const AClinicOperatingTable* Table = Cast<AClinicOperatingTable>(OperatingTable->GetChildActor()))
    {
        return Table->GetSpawnTransform();
    }
    return OperatingTable->GetComponentTransform();
}

void AStartingClinicRoom::ApplyLightingSettings()
{
    Moonlight->SetLightColor(AmbientColor);
    Moonlight->SetIntensity(AmbientIntensity);
    CandleLightA->SetLightColor(CandleColor);
    CandleLightA->SetIntensity(CandleIntensity);
    CandleLightB->SetLightColor(CandleColor);
    CandleLightB->SetIntensity(CandleIntensity * 0.75f);
}
