#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/BloodborneTypes.h"
#include "BloodbornePlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class USpringArmComponent;
class UStaminaComponent;
class UTargetLockComponent;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSprintStateChanged, bool, bNewSprinting);

/**
 * The Hunter. Owns locomotion (8-way walk/sprint), the Bloodborne-style dodge
 * (roll unlocked, quickstep locked-on), stamina wiring, lock-on, and the
 * third-person camera rig. Gameplay systems integrate through delegates;
 * animation state machine binding lands in Step 5 via GetMovementState().
 */
UCLASS(Blueprintable)
class BLOODBORNE2_API ABloodbornePlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ABloodbornePlayerCharacter();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    /** Enhanced Input handlers (bound by IA assets assigned in Blueprints). */
    void Move(const FInputActionValue& Value);
    void MoveCompleted(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void SprintPressed(const FInputActionValue& Value);
    void SprintReleased(const FInputActionValue& Value);
    void DodgePressed(const FInputActionValue& Value);
    void LockOnPressed(const FInputActionValue& Value);

    /** Step 5 animation state machine input. */
    UFUNCTION(BlueprintPure, Category="Bloodborne|Movement")
    EBloodborneMovementState GetMovementState() const { return MovementState; }

    UFUNCTION(BlueprintPure, Category="Bloodborne|Movement")
    bool IsInIFrames() const { return bHasIFrames; }

    UFUNCTION(BlueprintPure, Category="Bloodborne|Movement")
    bool IsSprinting() const { return bIsSprinting; }

    /** Pushes the character into the Stunned state (hit reactions, Step 4). */
    UFUNCTION(BlueprintCallable, Category="Bloodborne|Movement")
    void SetStunned(float Duration);

    UFUNCTION(BlueprintPure, Category="Bloodborne|Movement")
    FVector GetLastDodgeDirection() const { return LastDodgeDirection; }

    UFUNCTION(BlueprintPure, Category="Bloodborne|Components")
    UStaminaComponent* GetStaminaComponent() const { return Stamina; }

    UFUNCTION(BlueprintPure, Category="Bloodborne|Components")
    UTargetLockComponent* GetTargetLockComponent() const { return TargetLock; }

    UFUNCTION(BlueprintPure, Category="Bloodborne|Components")
    USpringArmComponent* GetCameraSpringArm() const { return SpringArm; }

    UFUNCTION(BlueprintPure, Category="Bloodborne|Components")
    UCameraComponent* GetBloodborneCamera() const { return Camera; }

    /** Fired whenever the locomotion pose changes (animation binding). */
    UPROPERTY(BlueprintAssignable, Category="Bloodborne|Movement")
    FOnMovementStateChanged OnMovementStateChanged;

    /** Fired when sprint starts/stops (audio/FOV/HUD hooks). */
    UPROPERTY(BlueprintAssignable, Category="Bloodborne|Movement")
    FOnSprintStateChanged OnSprintStateChanged;

protected:
    virtual void BeginPlay() override;

    void UpdateSprint(float DeltaTime);
    void UpdateFacing(float DeltaTime);
    void MaintainDodgeVelocity();
    void StartSprintInternal();
    void StopSprintInternal();
    void ExecuteDodge();
    FVector ResolveDodgeDirection(bool bLockedOn) const;
    void FinishDodge();
    void ClearIFrames();
    void EndStun();
    void EnterMovementState(EBloodborneMovementState NewState);
    FRotator GetMovementBasisRotation() const;
    FVector GetDirectionToTarget() const;

    // --- Components -----------------------------------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Bloodborne|Components")
    TObjectPtr<UStaminaComponent> Stamina;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Bloodborne|Components")
    TObjectPtr<UTargetLockComponent> TargetLock;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Bloodborne|Components")
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Bloodborne|Components")
    TObjectPtr<UCameraComponent> Camera;

    // --- Enhanced Input assets (assign in BP_BloodbornePlayerCharacter) ---
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Bloodborne|Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Bloodborne|Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Bloodborne|Input")
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Bloodborne|Input")
    TObjectPtr<UInputAction> DodgeAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Bloodborne|Input")
    TObjectPtr<UInputAction> LockOnAction;

    // --- Locomotion tuning ----------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Locomotion",
        meta=(ClampMin="10.0"))
    float WalkSpeed = 320.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Locomotion",
        meta=(ClampMin="10.0"))
    float SprintSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Locomotion",
        meta=(ClampMin="0.0"))
    float SprintStaminaDrainPerSecond = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Locomotion",
        meta=(ClampMin="0.05", ClampMax="1.0"))
    float SprintInputThreshold = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Locomotion",
        meta=(ClampMin="0.0"))
    float FacingInterpSpeed = 12.0f;

    /** Snap speed used while a dodge is playing out. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Locomotion",
        meta=(ClampMin="0.0"))
    float DodgeFacingInterpSpeed = 24.0f;

    // --- Dodge tuning -----------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Dodge",
        meta=(ClampMin="0.0"))
    float RollStaminaCost = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Dodge",
        meta=(ClampMin="0.05"))
    float RollDuration = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Dodge",
        meta=(ClampMin="0.0"))
    float RollLaunchSpeed = 620.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Dodge",
        meta=(ClampMin="0.0"))
    float RollIFrameDuration = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Dodge",
        meta=(ClampMin="0.0"))
    float QuickstepStaminaCost = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Dodge",
        meta=(ClampMin="0.05"))
    float QuickstepDuration = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Dodge",
        meta=(ClampMin="0.0"))
    float QuickstepLaunchSpeed = 1100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Dodge",
        meta=(ClampMin="0.0"))
    float QuickstepIFrameDuration = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bloodborne|Dodge")
    bool bAllowAirDodge = false;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Bloodborne|Movement",
        meta=(AllowPrivateAccess="true"))
    EBloodborneMovementState MovementState = EBloodborneMovementState::Normal;

    FVector2D CurrentMoveInput = FVector2D::ZeroVector;
    FVector LastWorldMoveDirection = FVector::ZeroVector;
    FVector LastDodgeDirection = FVector::ForwardVector;
    FVector DodgeVelocity = FVector::ZeroVector;

    FTimerHandle DodgeTimerHandle;
    FTimerHandle IFrameTimerHandle;
    FTimerHandle StunTimerHandle;

    bool bSprintHeld = false;
    bool bIsSprinting = false;
    bool bHasIFrames = false;
};
