#include "Components/TargetLockComponent.h"

#include "Components/SphereComponent.h"
#include "Core/LocalizationManager.h"
#include "Engine/GameInstance.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Targeting/BloodborneTargetable.h"

DEFINE_LOG_CATEGORY_STATIC(LogBloodborneTargeting, Log, All);

namespace
{
    const FName DetectionSphereName = TEXT("LockDetectionSphere");
    constexpr float DefaultFocusHeightOffset = 120.0f; // Chest framing for plain actors.
}

UTargetLockComponent::UTargetLockComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;

    DetectionSphere = CreateDefaultSubobject<USphereComponent>(DetectionSphereName);
    DetectionSphere->SetSphereRadius(DetectionRadius);
    DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    DetectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
    DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    DetectionSphere->SetGenerateOverlapEvents(false);
    DetectionSphere->SetHiddenInGame(true);
}

void UTargetLockComponent::BeginPlay()
{
    Super::BeginPlay();

    AttachDetectionVolume();
    SetComponentTickEnabled(bLockOnActive);
}

void UTargetLockComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    LockedTarget = nullptr;
    bLockOnActive = false;
    Super::EndPlay(EndPlayReason);
}

void UTargetLockComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bLockOnActive)
    {
        return;
    }

    ValidityTimer += DeltaTime;
    if (ValidityTimer >= ValidityCheckInterval)
    {
        ValidityTimer = 0.0f;
        UpdateLockValidity();
        UpdateLockedTargetVisibility();
    }
}

void UTargetLockComponent::ToggleLockOn()
{
    if (bLockOnActive)
    {
        ClearLockedTarget();
        return;
    }

    if (!bLockEnabled)
    {
        return;
    }

    if (!TryAcquireInitialTarget())
    {
        PublishLockNotification(FName(TEXT("hud.lock_on_no_target")));
    }
}

void UTargetLockComponent::CycleTarget(int32 Direction)
{
    if (!bLockOnActive || Direction == 0 || !IsValid(LockedTarget))
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return;
    }

    TArray<AActor*> Candidates;
    GatherCandidates(Candidates, true);
    if (Candidates.Num() < 2)
    {
        return;
    }

    // Sort by signed yaw relative to the owner's facing so "right" is stable.
    const FRotator OwnerRotation = Owner->GetActorRotation();
    const FVector OwnerLocation = Owner->GetActorLocation();
    Candidates.Sort([&OwnerRotation, &OwnerLocation](const AActor& Lhs, const AActor& Rhs)
    {
        const float LhsYaw = FRotator::NormalizeAxis(
            (Lhs.GetActorLocation() - OwnerLocation).Rotation().Yaw - OwnerRotation.Yaw);
        const float RhsYaw = FRotator::NormalizeAxis(
            (Rhs.GetActorLocation() - OwnerLocation).Rotation().Yaw - OwnerRotation.Yaw);
        return LhsYaw < RhsYaw;
    });

    const int32 CurrentIndex = Candidates.IndexOfByKey(LockedTarget);
    if (CurrentIndex == INDEX_NONE)
    {
        return;
    }

    const int32 NextIndex = (CurrentIndex + Direction + Candidates.Num()) % Candidates.Num();
    LockActor(Candidates[NextIndex]);
}

void UTargetLockComponent::SetLockEnabled(bool bNewEnabled)
{
    bLockEnabled = bNewEnabled;
    if (!bLockEnabled && bLockOnActive)
    {
        ClearLockedTarget();
    }
}

void UTargetLockComponent::ClearLockedTarget()
{
    if (!bLockOnActive && !IsValid(LockedTarget))
    {
        return;
    }

    const bool bHadTarget = IsValid(LockedTarget);
    bLockOnActive = false;
    LockedTarget = nullptr;
    bLockedTargetVisible = false;
    SetComponentTickEnabled(false);

    if (bHadTarget)
    {
        UE_LOG(LogBloodborneTargeting, Log, TEXT("%s unlocked target."), *GetNameSafe(GetOwner()));
        OnLockedTargetChanged.Broadcast(nullptr);
        OnTargetUnlocked.Broadcast();
    }
}

FVector UTargetLockComponent::GetLockedFocusLocation() const
{
    return ResolveFocusLocation(LockedTarget);
}

void UTargetLockComponent::AttachDetectionVolume()
{
    if (!DetectionSphere)
    {
        return;
    }

    DetectionSphere->SetSphereRadius(DetectionRadius);

    AActor* Owner = GetOwner();
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
    {
        DetectionSphere->AttachToComponent(OwnerCharacter->GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    }
    else if (Owner && Owner->GetRootComponent())
    {
        DetectionSphere->AttachToComponent(Owner->GetRootComponent(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    }
}

void UTargetLockComponent::UpdateLockValidity()
{
    if (!IsValid(LockedTarget) || !IsValid(GetOwner()))
    {
        ClearLockedTarget();
        return;
    }

    // Keep lock through brief wall occlusion (reticle fades instead); only
    // gameplay veto or leaving max range drops the lock.
    const FVector TargetLocation = ResolveFocusLocation(LockedTarget);
    const float DistanceSquared = (TargetLocation - GetOwner()->GetActorLocation()).SizeSquared();
    const bool bStillValid = IsCandidateValid(LockedTarget)
        && DistanceSquared <= FMath::Square(MaxLineOfSightDistance);
    if (!bStillValid)
    {
        UE_LOG(LogBloodborneTargeting, Log, TEXT("%s lost locked target %s."),
            *GetNameSafe(GetOwner()), *LockedTarget->GetName());

        LockedTarget = nullptr;
        bLockOnActive = false;
        bLockedTargetVisible = false;
        SetComponentTickEnabled(false);

        PublishLockNotification(FName(TEXT("hud.lock_on_lost_target")));
        OnLockedTargetChanged.Broadcast(nullptr);
        OnTargetUnlocked.Broadcast();
    }
}

void UTargetLockComponent::UpdateLockedTargetVisibility()
{
    bLockedTargetVisible = IsValid(LockedTarget) && HasLineOfSightTo(LockedTarget);
}

bool UTargetLockComponent::TryAcquireInitialTarget()
{
    TArray<AActor*> Candidates;
    GatherCandidates(Candidates, true);
    if (Candidates.IsEmpty())
    {
        return false;
    }

    AActor* BestCandidate = nullptr;
    float BestScore = TNumericLimits<float>::Max();
    for (AActor* Candidate : Candidates)
    {
        const float Score = ScoreCandidate(Candidate);
        if (Score < BestScore)
        {
            BestScore = Score;
            BestCandidate = Candidate;
        }
    }

    if (!IsValid(BestCandidate))
    {
        return false;
    }

    LockActor(BestCandidate);
    return true;
}

void UTargetLockComponent::GatherCandidates(TArray<AActor*>& OutCandidates,
    bool bRequireLineOfSight) const
{
    OutCandidates.Reset();

    AActor* Owner = GetOwner();
    UWorld* World = GetWorld();
    if (!IsValid(Owner) || !World || !DetectionSphere)
    {
        return;
    }

    const FVector ScanOrigin = DetectionSphere->GetComponentLocation();
    const FVector OwnerForward = Owner->GetActorForwardVector().GetSafeNormal2D();
    const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(AcquisitionHalfAngleDeg));

    // Pawns are always scannable; tuned actors opt in via the BBTargeting channel.
    FCollisionObjectQueryParams ObjectParams(ECC_Pawn);
    ObjectParams.AddObjectTypesToQuery(VisibilityTraceChannel);

    TArray<FOverlapResult> Overlaps;
    const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TargetLockScan), false, Owner);
    World->OverlapMultiByObjectType(Overlaps, ScanOrigin, FQuat::Identity, ObjectParams,
        FCollisionShape::MakeSphere(DetectionRadius), QueryParams);

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Candidate = Overlap.GetActor();
        if (!IsValid(Candidate) || Candidate == Owner)
        {
            continue;
        }
        if (IgnoredActorClass && Candidate->IsA(IgnoredActorClass))
        {
            continue;
        }
        if (!IsCandidateValid(Candidate))
        {
            continue;
        }

        const FVector ToCandidate = Candidate->GetActorLocation() - Owner->GetActorLocation();
        if (FMath::Abs(ToCandidate.Z) > MaxHeightDifference)
        {
            continue;
        }

        const FVector FlatCandidate = FVector(ToCandidate.X, ToCandidate.Y, 0.0f);
        if (FlatCandidate.IsNearlyZero())
        {
            continue;
        }
        if (FVector::DotProduct(OwnerForward, FlatCandidate.GetSafeNormal2D()) < CosHalfAngle)
        {
            continue;
        }
        if (bRequireLineOfSight && !HasLineOfSightTo(Candidate))
        {
            continue;
        }

        OutCandidates.AddUnique(Candidate);
    }
}

bool UTargetLockComponent::IsCandidateValid(AActor* Candidate) const
{
    if (!IsValid(Candidate) || Candidate == GetOwner())
    {
        return false;
    }

    if (Candidate->GetClass()->ImplementsInterface(UBloodborneTargetable::StaticClass()))
    {
        return IBloodborneTargetable::Execute_CanBeLockedOn(Candidate, GetOwner());
    }
    return true;
}

bool UTargetLockComponent::HasLineOfSightTo(AActor* Candidate) const
{
    const UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !IsValid(Owner) || !IsValid(Candidate))
    {
        return false;
    }

    const FVector TargetLocation = ResolveFocusLocation(Candidate);
    if ((TargetLocation - Owner->GetActorLocation()).SizeSquared() >
        FMath::Square(MaxLineOfSightDistance))
    {
        return false;
    }

    const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TargetLockLineOfSight), false, Owner);
    return !World->LineTraceTestByChannel(Owner->GetActorLocation(), TargetLocation,
        VisibilityTraceChannel, QueryParams);
}

float UTargetLockComponent::ScoreCandidate(AActor* Candidate) const
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner) || !IsValid(Candidate))
    {
        return TNumericLimits<float>::Max();
    }

    // Prefer candidates near the screen centre and close to the owner.
    const FVector ToCandidate = Candidate->GetActorLocation() - Owner->GetActorLocation();
    const float Distance = ToCandidate.Size();
    const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
        FVector::DotProduct(Owner->GetActorForwardVector(), ToCandidate.GetSafeNormal()),
        -1.0f, 1.0f)));
    return AngleDeg * 10.0f + Distance;
}

void UTargetLockComponent::LockActor(AActor* Candidate)
{
    if (!IsValid(Candidate) || Candidate == LockedTarget)
    {
        return;
    }

    LockedTarget = Candidate;
    bLockOnActive = true;
    bLockedTargetVisible = true;
    ValidityTimer = 0.0f;
    SetComponentTickEnabled(true);

    UE_LOG(LogBloodborneTargeting, Log, TEXT("%s locked onto %s."),
        *GetNameSafe(GetOwner()), *Candidate->GetName());
    OnLockedTargetChanged.Broadcast(Candidate);
    OnTargetLocked.Broadcast(Candidate);
}

void UTargetLockComponent::PublishLockNotification(FName LocalizationKey)
{
    AActor* Owner = GetOwner();
    const UGameInstance* GameInstance = Owner ? Owner->GetGameInstance() : nullptr;
    if (!GameInstance)
    {
        return;
    }

    const ULocalizationManager* Localization = GameInstance->GetSubsystem<ULocalizationManager>();
    const FText Message = Localization
        ? Localization->GetLocalizedText(LocalizationKey)
        : FText::FromName(LocalizationKey);
    OnLockNotification.Broadcast(Owner, Message);
}

FVector UTargetLockComponent::ResolveFocusLocation(AActor* Target) const
{
    if (!IsValid(Target))
    {
        AActor* Owner = GetOwner();
        return Owner ? Owner->GetActorLocation() + FVector::UpVector * DefaultFocusHeightOffset
                     : FVector::ZeroVector;
    }

    if (Target->GetClass()->ImplementsInterface(UBloodborneTargetable::StaticClass()))
    {
        return IBloodborneTargetable::Execute_GetTargetLockFocusLocation(Target);
    }

    FVector FocusLocation = Target->GetActorLocation();
    FocusLocation.Z += DefaultFocusHeightOffset;
    return FocusLocation;
}
