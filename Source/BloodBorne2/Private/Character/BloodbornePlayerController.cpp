#include "Character/BloodbornePlayerController.h"

#include "Camera/BloodborneCameraManager.h"
#include "Character/BloodbornePlayerCharacter.h"
#include "Components/StaminaComponent.h"
#include "Components/TargetLockComponent.h"
#include "Core/LocalizationManager.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogBloodbornePlayerController, Log, All);

ABloodbornePlayerController::ABloodbornePlayerController()
{
    PlayerCameraManagerClass = ABloodborneCameraManager::StaticClass();
    bShowMouseCursor = false;
    bEnableClickEvents = false;
}

void ABloodbornePlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (DefaultMappingContext)
    {
        if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
                LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                InputSubsystem->AddMappingContext(DefaultMappingContext, MappingPriority);
                UE_LOG(LogBloodbornePlayerController, Log, TEXT("Added default input mapping context."));
            }
        }
    }
    else
    {
        UE_LOG(LogBloodbornePlayerController, Warning,
            TEXT("No InputMappingContext assigned; assign IMC_Default on BP_BloodbornePlayerController."));
    }
}

UStaminaComponent* ABloodbornePlayerController::GetStaminaComponent() const
{
    return PossessedCharacter ? PossessedCharacter->GetStaminaComponent() : nullptr;
}

UTargetLockComponent* ABloodbornePlayerController::GetTargetLockComponent() const
{
    return PossessedCharacter ? PossessedCharacter->GetTargetLockComponent() : nullptr;
}

void ABloodbornePlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    PossessedCharacter = Cast<ABloodbornePlayerCharacter>(InPawn);
    if (PossessedCharacter)
    {
        BindGameplayEvents(PossessedCharacter);
    }
}

void ABloodbornePlayerController::OnUnPossess()
{
    UnbindGameplayEvents();
    PossessedCharacter = nullptr;
    Super::OnUnPossess();
}

void ABloodbornePlayerController::BindGameplayEvents(ABloodbornePlayerCharacter* InCharacter)
{
    if (UStaminaComponent* Stamina = InCharacter->GetStaminaComponent())
    {
        Stamina->OnStaminaDepleted.AddDynamic(this, &ABloodbornePlayerController::HandleStaminaDepleted);
    }

    if (UTargetLockComponent* TargetLock = InCharacter->GetTargetLockComponent())
    {
        TargetLock->OnTargetLocked.AddDynamic(this, &ABloodbornePlayerController::HandleTargetLocked);
        TargetLock->OnTargetUnlocked.AddDynamic(this, &ABloodbornePlayerController::HandleTargetUnlocked);
        TargetLock->OnLockNotification.AddDynamic(this, &ABloodbornePlayerController::HandleLockNotification);
    }
}

void ABloodbornePlayerController::UnbindGameplayEvents()
{
    if (PossessedCharacter)
    {
        if (UStaminaComponent* Stamina = PossessedCharacter->GetStaminaComponent())
        {
            Stamina->OnStaminaDepleted.RemoveDynamic(this, &ABloodbornePlayerController::HandleStaminaDepleted);
        }
        if (UTargetLockComponent* TargetLock = PossessedCharacter->GetTargetLockComponent())
        {
            TargetLock->OnTargetLocked.RemoveDynamic(this, &ABloodbornePlayerController::HandleTargetLocked);
            TargetLock->OnTargetUnlocked.RemoveDynamic(this, &ABloodbornePlayerController::HandleTargetUnlocked);
            TargetLock->OnLockNotification.RemoveDynamic(this, &ABloodbornePlayerController::HandleLockNotification);
        }
    }
}

void ABloodbornePlayerController::HandleStaminaDepleted()
{
    BroadcastLocalizedNotification(FName(TEXT("hud.stamina_depleted")));
}

void ABloodbornePlayerController::HandleTargetLocked(AActor* LockedTarget)
{
    BroadcastLocalizedNotification(FName(TEXT("hud.target_locked")));
}

void ABloodbornePlayerController::HandleTargetUnlocked()
{
    BroadcastLocalizedNotification(FName(TEXT("hud.target_unlocked")));
}

void ABloodbornePlayerController::HandleLockNotification(AActor* Source, FText Text)
{
    OnGameplayNotification.Broadcast(Text);
}

void ABloodbornePlayerController::BroadcastLocalizedNotification(FName LocalizationKey)
{
    const FText Message = ResolveLocalizedText(LocalizationKey);
    UE_LOG(LogBloodbornePlayerController, Verbose, TEXT("HUD notification: %s"), *Message.ToString());
    OnGameplayNotification.Broadcast(Message);
}

FText ABloodbornePlayerController::ResolveLocalizedText(FName LocalizationKey) const
{
    if (const UGameInstance* GameInstance = GetGameInstance())
    {
        if (const ULocalizationManager* Localization = GameInstance->GetSubsystem<ULocalizationManager>())
        {
            return Localization->GetLocalizedText(LocalizationKey);
        }
    }
    return FText::FromName(LocalizationKey);
}
