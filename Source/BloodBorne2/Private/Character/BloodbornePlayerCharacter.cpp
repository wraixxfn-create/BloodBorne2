#include "Character/BloodbornePlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaminaComponent.h"
#include "Components/TargetLockComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogBloodbornePlayer, Log, All);

ABloodbornePlayerCharacter::ABloodbornePlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

    // Facing is fully gameplay-driven (free rotation toward movement input or
    // the locked target); never inherit control rotation on the mesh.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = false;
    Movement->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
    Movement->MaxWalkSpeed = WalkSpeed;
    Movement->BrakingDecelerationWalking = 1500.0f;
    Movement->GroundFriction = 8.0f;
    Movement->AirControl = 0.35f;
    Movement->JumpZVelocity = 0.0f; // Bloodborne has no jump; keep ground play.

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
    SpringArm->SetupAttachment(GetCapsuleComponent());
    SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
    SpringArm->TargetArmLength = 380.0f;
    SpringArm->bUsePawnControlRotation = true;
    SpringArm->bDoCollisionTest = true;
    SpringArm->ProbeChannel = ECC_Camera;
    SpringArm->ProbeSize = 12.0f;
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 14.0f;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;
    Camera->FieldOfView = 80.0f;

    Stamina = CreateDefaultSubobject<UStaminaComponent>(TEXT("Stamina"));
    TargetLock = CreateDefaultSubobject<UTargetLockComponent>(TEXT("TargetLock"));

    // Mesh is assigned in BP_BloodbornePlayerCharacter (Step 5 rig/animation).
    GetMesh()->SetupAttachment(GetCapsuleComponent());
    GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
    GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
}

void ABloodbornePlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = WalkSpeed;
    }
}

void ABloodbornePlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UpdateSprint(DeltaTime);
    MaintainDodgeVelocity();
    UpdateFacing(DeltaTime);
}

void ABloodbornePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EnhancedInput)
    {
        Super::SetupPlayerInputComponent(PlayerInputComponent);
        return;
    }

    if (MoveAction)
    {
        EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this,
            &ABloodbornePlayerCharacter::Move);
        EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this,
            &ABloodbornePlayerCharacter::MoveCompleted);
        EnhancedInput->BindAction(MoveAction, ETriggerEvent::Canceled, this,
            &ABloodbornePlayerCharacter::MoveCompleted);
    }
    if (LookAction)
    {
        EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this,
            &ABloodbornePlayerCharacter::Look);
    }
    if (SprintAction)
    {
        EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this,
            &ABloodbornePlayerCharacter::SprintPressed);
        EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this,
            &ABloodbornePlayerCharacter::SprintReleased);
        EnhancedInput->BindAction(SprintAction, ETriggerEvent::Canceled, this,
            &ABloodbornePlayerCharacter::SprintReleased);
    }
    if (DodgeAction)
    {
        EnhancedInput->BindAction(DodgeAction, ETriggerEvent::Started, this,
            &ABloodbornePlayerCharacter::DodgePressed);
    }
    if (LockOnAction)
    {
        EnhancedInput->BindAction(LockOnAction, ETriggerEvent::Started, this,
            &ABloodbornePlayerCharacter::LockOnPressed);
    }
}

void ABloodbornePlayerCharacter::Move(const FInputActionValue& Value)
{
    CurrentMoveInput = Value.Get<FVector2D>();

    if (MovementState != EBloodborneMovementState::Normal || CurrentMoveInput.IsNearlyZero())
    {
        return; // Dodges/stuns play out on their committed direction.
    }

    const FRotator BasisRotation = GetMovementBasisRotation();
    const FVector ForwardDirection = FRotationMatrix(BasisRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(BasisRotation).GetUnitAxis(EAxis::Y);
    const FVector MoveDirection = (ForwardDirection * CurrentMoveInput.Y
        + RightDirection * CurrentMoveInput.X).GetSafeNormal();

    LastWorldMoveDirection = MoveDirection;
    AddMovementInput(MoveDirection, CurrentMoveInput.Size());
}

void ABloodbornePlayerCharacter::MoveCompleted(const FInputActionValue& Value)
{
    CurrentMoveInput = FVector2D::ZeroVector;
}

void ABloodbornePlayerCharacter::Look(const FInputActionValue& Value)
{
    // Locked-on camera framing is driven by ABloodborneCameraManager.
    if (TargetLock && TargetLock->HasLockedTarget())
    {
        return;
    }

    const FVector2D LookAxis = Value.Get<FVector2D>();
    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        PlayerController->AddYawInput(LookAxis.X);
        PlayerController->AddPitchInput(LookAxis.Y);
    }
}

void ABloodbornePlayerCharacter::SprintPressed(const FInputActionValue& Value)
{
    bSprintHeld = true;
}

void ABloodbornePlayerCharacter::SprintReleased(const FInputActionValue& Value)
{
    bSprintHeld = false;
}

void ABloodbornePlayerCharacter::DodgePressed(const FInputActionValue& Value)
{
    ExecuteDodge();
}

void ABloodbornePlayerCharacter::LockOnPressed(const FInputActionValue& Value)
{
    if (TargetLock)
    {
        TargetLock->ToggleLockOn();
    }
}

void ABloodbornePlayerCharacter::SetStunned(float Duration)
{
    if (MovementState == EBloodborneMovementState::Rolling
        || MovementState == EBloodborneMovementState::Dashing)
    {
        GetWorldTimerManager().ClearTimer(DodgeTimerHandle);
        GetWorldTimerManager().ClearTimer(IFrameTimerHandle);
        bHasIFrames = false;
    }
    if (bIsSprinting)
    {
        StopSprintInternal();
    }

    EnterMovementState(EBloodborneMovementState::Stunned);
    GetWorldTimerManager().ClearTimer(StunTimerHandle);
    if (Duration > 0.0f)
    {
        GetWorldTimerManager().SetTimer(StunTimerHandle, this,
            &ABloodbornePlayerCharacter::EndStun, Duration, false);
    }
}

void ABloodbornePlayerCharacter::UpdateSprint(float DeltaTime)
{
    if (MovementState != EBloodborneMovementState::Normal)
    {
        if (bIsSprinting)
        {
            StopSprintInternal();
        }
        return;
    }

    const bool bMovingEnough = CurrentMoveInput.Size() >= SprintInputThreshold;
    const bool bStaminaAllowsSprint = Stamina && !Stamina->IsExhausted()
        && Stamina->HasStamina(Stamina->GetSprintExhaustionFloor() + KINDA_SMALL_NUMBER);
    const bool bCanSprint = bSprintHeld && bMovingEnough && bStaminaAllowsSprint;

    if (bCanSprint)
    {
        if (!bIsSprinting)
        {
            StartSprintInternal();
        }

        Stamina->DrainStaminaPerSecond(SprintStaminaDrainPerSecond, DeltaTime);
        if (UCharacterMovementComponent* Movement = GetCharacterMovement())
        {
            Movement->MaxWalkSpeed = SprintSpeed;
        }

        // Hit the floor -> hunter drops back to a walk until stamina recovers.
        if (Stamina->GetCurrentStamina() <= Stamina->GetSprintExhaustionFloor() + KINDA_SMALL_NUMBER)
        {
            StopSprintInternal();
        }
        return;
    }

    if (bIsSprinting)
    {
        StopSprintInternal();
    }
}

void ABloodbornePlayerCharacter::StartSprintInternal()
{
    bIsSprinting = true;
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = SprintSpeed;
    }
    OnSprintStateChanged.Broadcast(true);
}

void ABloodbornePlayerCharacter::StopSprintInternal()
{
    bIsSprinting = false;
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = WalkSpeed;
    }
    OnSprintStateChanged.Broadcast(false);
}

void ABloodbornePlayerCharacter::ExecuteDodge()
{
    if (MovementState != EBloodborneMovementState::Normal)
    {
        return;
    }
    if (!bAllowAirDodge && GetCharacterMovement()->IsFalling())
    {
        return;
    }

    const bool bLockedOn = TargetLock && TargetLock->HasLockedTarget();
    const float StaminaCost = bLockedOn ? QuickstepStaminaCost : RollStaminaCost;
    if (!Stamina || !Stamina->TryConsumeStamina(StaminaCost))
    {
        UE_LOG(LogBloodbornePlayer, Verbose, TEXT("Dodge refused: not enough stamina (%.1f required)."),
            StaminaCost);
        return; // Bloodborne gates evasions behind stamina.
    }

    if (bIsSprinting)
    {
        StopSprintInternal();
    }

    LastDodgeDirection = ResolveDodgeDirection(bLockedOn);
    const float LaunchSpeed = bLockedOn ? QuickstepLaunchSpeed : RollLaunchSpeed;
    DodgeVelocity = LastDodgeDirection * LaunchSpeed;

    EnterMovementState(bLockedOn ? EBloodborneMovementState::Dashing
                                 : EBloodborneMovementState::Rolling);

    // Brief invincibility window (i-frames).
    bHasIFrames = true;
    const float IFrameDuration = bLockedOn ? QuickstepIFrameDuration : RollIFrameDuration;
    const float DodgeDuration = bLockedOn ? QuickstepDuration : RollDuration;

    FTimerManager& Timers = GetWorldTimerManager();
    Timers.SetTimer(IFrameTimerHandle, this, &ABloodbornePlayerCharacter::ClearIFrames,
        IFrameDuration, false);
    Timers.SetTimer(DodgeTimerHandle, this, &ABloodbornePlayerCharacter::FinishDodge,
        DodgeDuration, false);

    LaunchCharacter(DodgeVelocity, true, false);
    UE_LOG(LogBloodbornePlayer, Verbose, TEXT("%s executed (%s)."),
        bLockedOn ? TEXT("Quickstep") : TEXT("Roll"), *LastDodgeDirection.ToString());
}

FVector ABloodbornePlayerCharacter::ResolveDodgeDirection(bool bLockedOn) const
{
    if (bLockedOn)
    {
        // Direction relative to the locked target: Y = toward/away, X = strafe.
        const FVector ToTarget = GetDirectionToTarget();
        if (CurrentMoveInput.IsNearlyZero())
        {
            return -ToTarget; // Neutral dodge steps away from the threat.
        }
        const FVector Right = FVector::CrossProduct(FVector::UpVector, ToTarget);
        return (ToTarget * CurrentMoveInput.Y + Right * CurrentMoveInput.X).GetSafeNormal();
    }

    // Camera-relative direction; neutral input rolls backwards.
    const FRotator BasisRotation = GetMovementBasisRotation();
    const FVector ForwardDirection = FRotationMatrix(BasisRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(BasisRotation).GetUnitAxis(EAxis::Y);
    if (CurrentMoveInput.IsNearlyZero())
    {
        return -ForwardDirection;
    }
    return (ForwardDirection * CurrentMoveInput.Y + RightDirection * CurrentMoveInput.X).GetSafeNormal();
}

void ABloodbornePlayerCharacter::FinishDodge()
{
    if (MovementState == EBloodborneMovementState::Rolling
        || MovementState == EBloodborneMovementState::Dashing)
    {
        EnterMovementState(EBloodborneMovementState::Normal);
    }
}

void ABloodbornePlayerCharacter::ClearIFrames()
{
    bHasIFrames = false;
}

void ABloodbornePlayerCharacter::EndStun()
{
    if (MovementState == EBloodborneMovementState::Stunned)
    {
        EnterMovementState(EBloodborneMovementState::Normal);
    }
}

void ABloodbornePlayerCharacter::EnterMovementState(EBloodborneMovementState NewState)
{
    if (MovementState == NewState)
    {
        return;
    }

    UE_LOG(LogBloodbornePlayer, Log, TEXT("Movement state: %s -> %s"),
        *UEnum::GetDisplayValueAsText(MovementState).ToString(),
        *UEnum::GetDisplayValueAsText(NewState).ToString());

    MovementState = NewState;
    OnMovementStateChanged.Broadcast(NewState);
}

void ABloodbornePlayerCharacter::MaintainDodgeVelocity()
{
    if (MovementState != EBloodborneMovementState::Rolling
        && MovementState != EBloodborneMovementState::Dashing)
    {
        return;
    }

    // Keep the launch speed constant for the whole dodge so distance tuning
    // is deterministic (distance = launch speed * duration).
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        FVector Velocity = Movement->Velocity;
        Velocity.X = DodgeVelocity.X;
        Velocity.Y = DodgeVelocity.Y;
        Movement->Velocity = Velocity;
    }
}

void ABloodbornePlayerCharacter::UpdateFacing(float DeltaTime)
{
    const bool bLockedOn = TargetLock && TargetLock->HasLockedTarget();
    float DesiredYaw = GetActorRotation().Yaw;

    switch (MovementState)
    {
    case EBloodborneMovementState::Rolling:
        DesiredYaw = LastDodgeDirection.Rotation().Yaw;
        break;
    case EBloodborneMovementState::Dashing:
        DesiredYaw = bLockedOn ? GetDirectionToTarget().Rotation().Yaw
                               : LastDodgeDirection.Rotation().Yaw;
        break;
    case EBloodborneMovementState::Stunned:
        return; // Hold facing while staggered.
    case EBloodborneMovementState::Normal:
    default:
        if (bLockedOn)
        {
            DesiredYaw = GetDirectionToTarget().Rotation().Yaw;
        }
        else if (CurrentMoveInput.SizeSquared() > KINDA_SMALL_NUMBER)
        {
            DesiredYaw = LastWorldMoveDirection.Rotation().Yaw;
        }
        else
        {
            return; // Idle: keep current facing.
        }
        break;
    }

    const float InterpSpeed = MovementState == EBloodborneMovementState::Normal
        ? FacingInterpSpeed
        : DodgeFacingInterpSpeed;

    const FRotator CurrentRotation(0.0f, GetActorRotation().Yaw, 0.0f);
    const FRotator DesiredRotation(0.0f, DesiredYaw, 0.0f);
    const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation,
        DeltaTime, InterpSpeed);
    SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
}

FRotator ABloodbornePlayerCharacter::GetMovementBasisRotation() const
{
    if (TargetLock && TargetLock->HasLockedTarget())
    {
        // Forward = toward the locked target (strafe orbit around it).
        return GetDirectionToTarget().Rotation();
    }

    const AController* PlayerController = GetController();
    const float ControlYaw = PlayerController
        ? PlayerController->GetControlRotation().Yaw
        : GetActorRotation().Yaw;
    return FRotator(0.0f, ControlYaw, 0.0f);
}

FVector ABloodbornePlayerCharacter::GetDirectionToTarget() const
{
    if (!TargetLock || !TargetLock->HasLockedTarget())
    {
        return GetActorForwardVector().GetSafeNormal2D();
    }

    const FVector ToTarget = TargetLock->GetLockedFocusLocation() - GetActorLocation();
    return ToTarget.GetSafeNormal2D();
}
