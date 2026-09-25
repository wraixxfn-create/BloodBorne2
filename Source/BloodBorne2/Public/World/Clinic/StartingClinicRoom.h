#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StartingClinicRoom.generated.h"

class AClinicOperatingTable;
class UChildActorComponent;
class UPointLightComponent;
class URectLightComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Modular opening sickroom assembly. This actor owns layout, gameplay children,
 * and baseline lighting; art meshes/materials are assigned in a Blueprint child.
 */
UCLASS(Blueprintable)
class BLOODBORNE2_API AStartingClinicRoom : public AActor
{
    GENERATED_BODY()

public:
    AStartingClinicRoom();
    virtual void OnConstruction(const FTransform& Transform) override;

    UFUNCTION(BlueprintPure, Category="Clinic|Spawn")
    FTransform GetPlayerSpawnTransform() const;

protected:
    void ApplyLightingSettings();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Structure")
    TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Structure")
    TObjectPtr<UStaticMeshComponent> RoomShell;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Structure")
    TObjectPtr<UStaticMeshComponent> ArchedWindows;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Structure")
    TObjectPtr<UStaticMeshComponent> Dressing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Gameplay")
    TObjectPtr<UChildActorComponent> OperatingTable;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Gameplay")
    TObjectPtr<UChildActorComponent> LockedDoor;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Gameplay")
    TObjectPtr<UChildActorComponent> FloorNote;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Lighting")
    TObjectPtr<URectLightComponent> Moonlight;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Lighting")
    TObjectPtr<UPointLightComponent> CandleLightA;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Lighting")
    TObjectPtr<UPointLightComponent> CandleLightB;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clinic|Lighting")
    FLinearColor AmbientColor = FLinearColor(0.10f, 0.18f, 0.30f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clinic|Lighting", meta=(ClampMin="0.0"))
    float AmbientIntensity = 1.25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clinic|Lighting")
    FLinearColor CandleColor = FLinearColor(1.0f, 0.42f, 0.12f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clinic|Lighting", meta=(ClampMin="0.0"))
    float CandleIntensity = 850.0f;
};
