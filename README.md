# BloodBorne2 — Modular Souls-like Foundation (UE5 C++)

Unreal Engine 5 C++ foundation for a modular dark-fantasy action RPG.

- **Step 1** — runtime EN/IT JSON localization, event-driven interaction layer, opening clinic sickroom assembly.
- **Step 2** — Bloodborne-style player controller: Enhanced Input, 8-way walk/sprint, dynamic dodge (Roll ↔ Quickstep), stamina system with exhaustion penalty, custom lock-on camera, and the `EBloodborneMovementState` contract for the Step 5 animation state machine.

## Repository layout

```text
BloodBorne2.uproject
Config/
  DefaultEngine.ini              # BBTargeting / BBInteraction trace channels
  DefaultGame.ini
  DefaultInput.ini               # Enhanced Input default classes
Content/
  Localization/
    en.json
    it.json
  BloodBorne2/                   # Create editor assets here (not generated yet)
    Blueprints/                  # BP_BBGameMode, BP_BloodbornePlayerCharacter,
                                 # BP_BloodbornePlayerController, World/Clinic/
    Input/                       # IA_* actions, IMC_Default (create per Step 2 guide)
    Maps/
    Materials/
    Meshes/Environment/Clinic/
    UI/
Source/
  BloodBorne2.Target.cs
  BloodBorne2Editor.Target.cs
  BloodBorne2/
    BloodBorne2.Build.cs         # +EnhancedInput dependency (Step 2)
    Public/
      Core/LocalizationManager.h
      Core/BloodborneTypes.h     # movement state enum, trace channels, targetable iface
      Components/StaminaComponent.h
      Components/TargetLockComponent.h
      Camera/BloodborneCameraManager.h
      Character/BloodbornePlayerCharacter.h
      Character/BloodbornePlayerController.h
      Interaction/InteractableObject.h
      Targeting/BloodborneTargetable.h
      World/Clinic/*.h
    Private/                     # matching .cpp files
```

Generated Unreal folders (`Binaries`, `Intermediate`, `Saved`, and `DerivedDataCache`) are intentionally ignored. Commit source assets, `.uproject`, config, C++ source, and localization JSON.

## Open and build

1. Install UE 5.3 or update `EngineAssociation` and target `IncludeOrderVersion` together for the team engine version.
2. Right-click `BloodBorne2.uproject` and generate project files (or run the platform-specific Unreal project generator).
3. Build the `BloodBorne2Editor` Development target.
4. Open the project and create `/Game/BloodBorne2/Maps/Clinic_Sickroom`.

The Build.cs runtime dependencies and packaging setting stage both JSON files as loose Non-UFS content, allowing the runtime loader to use `ProjectContentDir()/Localization`. If localization files are moved, update both locations.

---

## Step 1 recap

### Runtime localization

`ULocalizationManager` is a `UGameInstanceSubsystem`; Unreal creates one for every game instance. Obtain it in C++ with:

```cpp
ULocalizationManager* Localization =
    GetGameInstance()->GetSubsystem<ULocalizationManager>();

Localization->SetLanguage(TEXT("it"));
const FText Label = Localization->GetLocalizedText(TEXT("ui.examine"));
```

In Blueprint use **Get Game Instance Subsystem** (`LocalizationManager`), then call **Set Language** or **Get Localized Text**. Bind UI widgets to `OnLanguageChanged` and refresh displayed text when it fires. The system:

- normalizes `it-IT`/`it_US` style codes to `it`;
- keeps the current dictionary if a new file is invalid;
- falls back per key to English, then displays the key for easy QA;
- recursively flattens JSON groups (`notes.paleblood_scribble.body`);
- rejects unsupported languages rather than silently choosing one.

Add strings in matching nested locations in both JSON files. Values must be strings.

### Interaction flow

`AInteractableObject` is the base gameplay actor. A player interaction component should line trace, cast the hit actor to `AInteractableObject`, check `CanInteract`, and invoke `Interact`. UI/audio systems subscribe to delegates rather than being hard-coupled to actors:

- `OnInteracted(Interactor)` — generic gameplay interaction event.
- `OnInteractionText(Interactor, Text)` — localized notification/note content.

`AClinicDoor` emits the localized locked message while locked and exposes a Blueprint `OnDoorOpened` event when unlocked. `AClinicNote` emits the localized scribble. `AClinicOperatingTable` provides the deterministic player/cinematic spawn transform (`GetPlayerSpawnTransform()`).

---

## Step 2 — Player Controller, Stamina, Lock-On, Dynamic Dodge

### Class overview

| Class | Role |
|---|---|
| `ABloodbornePlayerCharacter` | The hunter: 8-way movement, sprint, dynamic dodge (Roll unlocked / Quickstep locked-on), facing control, i-frames, movement state machine, camera rig (spring arm + camera). |
| `ABloodbornePlayerController` | Enhanced Input mapping context install, camera manager install, localized HUD notification relay (`OnGameplayNotification`). |
| `UStaminaComponent` | Stat-like stamina pool: consumption, sprint drain with floor, regeneration, exhaustion penalty delay, delegates for UI. |
| `UTargetLockComponent` | Candidate scanning (sphere + cone + line of sight), scoring, cycling, validity upkeep, lock state events. |
| `ABloodborneCameraManager` | Camera collision filtering on the spring arm; lock-on framing (camera centered on the player↔target axis, arm length frames both actors). |
| `IBloodborneTargetable` | Opt-in interface for tuned lock-on candidates (focus point, veto). |
| `EBloodborneMovementState` | `Normal` / `Rolling` / `Dashing` / `Stunned` — animation state machine input for Step 5. |

Everything is wired through delegates; no component reaches into widgets, and no class polls another for state it can subscribe to.

```text
                        UStaminaComponent
                        OnStaminaChanged ──────────────► HUD stamina bar (Step 3)
                        OnStaminaDepleted ──► ABloodbornePlayerController ─► localized "hud.stamina_depleted"
                        OnStaminaRegenerationStarted ──► HUD/audio (Step 3)

                        UTargetLockComponent
                        OnTargetLocked/OnTargetUnlocked ─► controller ─► localized lock messages
                        OnLockedTargetChanged ──────────► camera/HUD reticle
                        OnLockNotification (FText) ─────► controller relay

                        ABloodbornePlayerCharacter
                        OnMovementStateChanged ─────────► Step 5 AnimBP state machine
                        OnSprintStateChanged ───────────► audio/FOV/HUD

                        ABloodbornePlayerController
                        OnGameplayNotification (FText) ─► HUD notification feed (Step 3)
```

### Dynamic dodge logic

`ABloodbornePlayerCharacter::ExecuteDodge()` is a single entry point that branches on lock state:

| | Unlocked: `Roll` | Locked-on: `Quickstep` |
|---|---|---|
| Direction | Camera-relative movement input; neutral input rolls backwards | Relative to locked target: Y = toward/away, X = strafe; neutral input steps away |
| Defaults | 15 stamina, 0.65 s, 620 u/s, 0.35 s i-frames | 12 stamina, 0.45 s, 1100 u/s, 0.30 s i-frames |
| Facing | Turns into the roll direction | Keeps facing the target (strafe-dash feel) |
| State | `EBloodborneMovementState::Rolling` | `EBloodborneMovementState::Dashing` |

Dodge distance is deterministic: horizontal velocity is pinned for the whole dodge, so `distance = launch speed × duration` (all values are `EditAnywhere`). A dodge is refused when stamina cannot pay its cost — Bloodborne gates evasions behind stamina. `IsInIFrames()` is the hook the Step 4 damage system checks before applying hits.

### Stamina behavior

- `TryConsumeStamina` — all-or-nothing spends (dodges, future attacks). Any spend restarts `RegenerationDelay`.
- `DrainStaminaPerSecond` — sprint tick drain with a hard floor (`SprintExhaustionFloor`) so the hunter always keeps a sliver for dodging.
- Reaching 0 fires `OnStaminaDepleted`, sets the exhausted flag, and applies `ExhaustionPenaltyDelay` before regeneration resumes (`OnStaminaRegenerationStarted` fires at that moment).
- `SetRegenerationBlocked(true)` suspends regen for future aiming/stagger systems.

### Lock-on targeting

`UTargetLockComponent` scans a sphere (`DetectionRadius`, default 20 m) around the character. Candidates must:

1. be a pawn **or** set a collision response of **Overlap** on the `BBTargeting` channel (`ECC_GameTraceChannel1`);
2. sit inside `MaxHeightDifference` and the `AcquisitionHalfAngleDeg` cone in front of the player;
3. pass a `BBTargeting` line-of-sight trace at acquisition time;
4. not veto via `IBloodborneTargetable::CanBeLockedOn`.

Best candidate is scored by screen-centre angle (×10) plus distance. While locked, validity re-checks every `ValidityCheckInterval`: gameplay veto or leaving `MaxLineOfSightDistance` unlocks (with a localized "Target lost" notification), while brief wall occlusion only fades `IsLockedTargetVisible()` for HUD reticle dimming. `ToggleLockOn()` is the input entry point; `CycleTarget(±1)` orbits selection left/right.

While locked, `Look` input is consumed by `ABloodborneCameraManager`, which interpolates the control rotation onto the player→target axis (pitch `LockedCameraPitch`) and frames both actors by clamping the spring arm to `distance × LockedArmLengthRatio`. The character strafes relative to the target (forward = toward target) and always faces it.

### Enhanced Input setup (editor, one-time)

Create in `/Game/BloodBorne2/Input/`:

| Asset | Type | Triggers/Value |
|---|---|---|
| `IA_Move` | Input Action | Value Type **Axis2D (Vector2D)** |
| `IA_Look` | Input Action | Value Type **Axis2D (Vector2D)** |
| `IA_Sprint` | Input Action | Value Type **Digital (bool)** |
| `IA_Dodge` | Input Action | Value Type **Digital (bool)** |
| `IA_LockOn` | Input Action | Value Type **Digital (bool)** |

Create `IMC_Default` (Input Mapping Context) and map:

| Action | Keyboard | Gamepad |
|---|---|---|
| `IA_Move` | W/S → Keyboard with **Swizzle Input Axis Values (YXZ)** + **Negate** on S; A/D likewise | Left Stick → **Negate** on down input |
| `IA_Look` | Mouse XY 2D-Axis (modifier; **Negate** Y if inverted) | Right Stick 2D-Axis |
| `IA_Sprint` | Left Shift | Left Shoulder (or L3) |
| `IA_Dodge` | Space | Face Button Right (Circle) |
| `IA_LockOn` | Q / Middle Mouse | Left Thumbstick click |

Then create the Blueprint wiring:

1. `BP_BloodbornePlayerCharacter` (parent `BloodbornePlayerCharacter`) — assign `MoveAction`, `LookAction`, `SprintAction`, `DodgeAction`, `LockOnAction`; tune locomotion/dodge values; assign the hunter skeleton/mesh here (Step 5).
2. `BP_BloodbornePlayerController` (parent `BloodbornePlayerController`) — assign `DefaultMappingContext = IMC_Default`.
3. `BP_BBGameMode` (parent `GameModeBase`) — set **Player Controller Class** = `BP_BloodbornePlayerController` and **Default Pawn Class** = `BP_BloodbornePlayerCharacter`. `PlayerCameraManagerClass` is already set in C++ (`ABloodborneCameraManager`).
4. Assign `BP_BBGameMode` as the map/world default (see commented lines in `Config/DefaultEngine.ini`).

No mouse cursor is shown; input mode is game-only by default.

### Collision channels

`Config/DefaultEngine.ini` registers two trace channels referenced from `Core/BloodborneTypes.h`:

- `BBTargeting` (`ECC_GameTraceChannel1`, macro `ECC_BB_TARGETING`) — lock-on LOS traces. World geometry blocks it by default. Step 5 enemies opt in as lock candidates by setting their capsule/mesh response to **Overlap** for this channel (or simply being a pawn — pawns are always scannable).
- `BBInteraction` (`ECC_GameTraceChannel2`) — reserved for interaction traces.

Camera wall protection uses the built-in `ECC_Camera` channel: the spring arm traces with `ProbeSize = 12` and pulls the camera inside blocking walls; `ABloodborneCameraManager::EnsureCameraCollisionProfile()` enforces this at runtime. For dense dressing, consider meshes that ignore/overlap `Camera` plus occlusion-fade materials.

### Implementing lock-on enemies (Step 5 preview)

```cpp
// Enemy.h
#include "Targeting/BloodborneTargetable.h"

UCLASS()
class ABeastEnemy : public ACharacter, public IBloodborneTargetable
{
    GENERATED_BODY()
public:
    virtual FVector GetTargetLockFocusLocation_Implementation() override; // head socket
    virtual bool CanBeLockedOn_Implementation(AActor* RequestingActor) override; // false while dying
};
```

Then set the capsule collision response for `BBTargeting` to **Overlap** (optional — pawns are scannable anyway).

### Step 2 acceptance checklist

- PIE: 8-way walk; hold Sprint to run with visible stamina drain; releasing or idling regenerates after the delay.
- Sprint to the stamina floor: sprint auto-drops to walk; dodge remains possible until true exhaustion; "Stamina Depleted" / "Vigore esaurito" notification fires at zero.
- Dodge unlocked: directional roll in all 8 camera-relative directions; neutral input rolls backwards.
- Press Lock-On with no enemies: localized "No target in range" appears; camera unchanged.
- With a test pawn (or Step 5 enemy) in range: lock centers camera between hunter and target; movement strafes around it; dodge becomes Quickstep (sharp, short, target-relative); losing LOS at range unlocks with "Target lost".
- I-frames: a future/scratch damage source applied during the dodge window must not register (`IsInIFrames()` true).
- Switch language EN↔IT at runtime: all Step 2 notifications flip instantly (`hud.*` keys).
- Camera never clips through clinic walls while orbiting.
- `OnMovementStateChanged` broadcasts Normal → Rolling/Dashing → Normal in logs (`LogBloodbornePlayer`) for the Step 5 AnimBP.
