# BloodBorne2 — Step 1 Foundation

Unreal Engine 5 C++ foundation for a modular dark-fantasy action RPG. Step 1 supplies runtime EN/IT JSON localization, an event-driven interaction layer, and the opening clinic sickroom assembly.

## Repository layout

```text
BloodBorne2.uproject
Config/
  DefaultGame.ini
Content/
  Localization/
    en.json
    it.json
  BloodBorne2/                 # Create editor assets here (not generated yet)
    Blueprints/World/Clinic/
    Maps/
    Materials/
    Meshes/Environment/Clinic/
    UI/
Source/
  BloodBorne2.Target.cs
  BloodBorne2Editor.Target.cs
  BloodBorne2/
    BloodBorne2.Build.cs
    Public/
      Core/LocalizationManager.h
      Interaction/InteractableObject.h
      World/Clinic/*.h
    Private/
      Core/LocalizationManager.cpp
      Interaction/InteractableObject.cpp
      World/Clinic/*.cpp
```

Generated Unreal folders (`Binaries`, `Intermediate`, `Saved`, and `DerivedDataCache`) are intentionally ignored. Commit source assets, `.uproject`, config, C++ source, and localization JSON.

## Open and build

1. Install UE 5.3 or update `EngineAssociation` and target `IncludeOrderVersion` together for the team engine version.
2. Right-click `BloodBorne2.uproject` and generate project files (or run the platform-specific Unreal project generator).
3. Build the `BloodBorne2Editor` Development target.
4. Open the project and create `/Game/BloodBorne2/Maps/Clinic_Sickroom`.

The Build.cs runtime dependencies and packaging setting stage both JSON files as loose Non-UFS content, allowing the runtime loader to use `ProjectContentDir()/Localization`. If localization files are moved, update both locations.

## Runtime localization

`ULocalizationManager` is a `UGameInstanceSubsystem`; Unreal creates one for every game instance. Obtain it in C++ with:

```cpp
ULocalizationManager* Localization =
    GetGameInstance()->GetSubsystem<ULocalizationManager>();

Localization->SetLanguage(TEXT("it"));
const FText Label = Localization->GetLocalizedText(TEXT("ui.examine"));
```

In Blueprint use **Get Game Instance Subsystem** (`LocalizationManager`), then call **Set Language** or **Get Localized Text**. Bind UI widgets to `OnLanguageChanged` and refresh displayed text when it fires. The system:

- normalizes `it-IT`/`it_IT` to `it`;
- keeps the current dictionary if a new file is invalid;
- falls back per key to English, then displays the key for easy QA;
- recursively flattens JSON groups (`notes.paleblood_scribble.body`);
- rejects unsupported languages rather than silently choosing one.

Add strings in matching nested locations in both JSON files. Values must be strings. The JSON system is appropriate for game-owned runtime copy; UE's native String Tables/LOCTEXT should still be used for editor/tool copy that requires gather-text workflows.

## Interaction flow

`AInteractableObject` is the base gameplay actor. A player interaction component should line trace, cast the hit actor to `AInteractableObject`, check `CanInteract`, and invoke `Interact`. UI/audio systems subscribe to delegates rather than being hard-coupled to actors:

- `OnInteracted(Interactor)` — generic gameplay interaction event.
- `OnInteractionText(Interactor, Text)` — localized notification/note content.

`AClinicDoor` emits the localized locked message while locked and exposes a Blueprint `OnDoorOpened` event when unlocked. `AClinicNote` emits the localized scribble. `AClinicOperatingTable` provides the deterministic player/cinematic spawn transform.

## Clinic sickroom art and Blueprint assembly

Create `BP_StartingClinicRoom` derived from `AStartingClinicRoom` and assign meshes to its inherited components:

| Component | Recommended asset treatment |
|---|---|
| `RoomShell_CrackedStone` | Nanite cracked stone shell, damp roughness variation, dark grout decals |
| `TallArchedWindows` | Tall Victorian arches, dirty translucent glass, exterior moon card |
| `VictorianRoomDressing` | Worn wood tables, bottles, surgical tools, sheets and blood decals |
| Child actors | Create Blueprint subclasses to assign the operating table, door, and note meshes |

The actor creates one cool blue-grey rectangular moon source and two warm candle point lights. Defaults are editable per instance. Recommended starting grade:

- Ambient/moon: `#1A2E4D`, low exposure contribution, broad soft shadows.
- Candles: approximately `#FF6B1F`, inverse-square falloff, 360–430 cm radius.
- Post process: exposure locked near EV 2–4, slightly desaturated, cool shadows, subtle bloom and vignette.
- Use actual candle flame Niagara systems separately; do not make gameplay lights tick every frame. Add restrained Blueprint flicker with a material/light curve only where visible.

Place the room actor at world origin. Use `GetPlayerSpawnTransform()` after child actors have initialized (for example from GameMode `BeginPlay`) to place the player. Mesh references deliberately remain out of C++ so environment art can be replaced without recompilation.

## Step 1 acceptance checklist

- Run PIE in English; door and note emit English strings.
- Call `SetLanguage("it")`; open UI refreshes and interactions emit Italian strings.
- Test `it-IT`, unsupported codes, and a missing Italian key (English fallback).
- Confirm player spawn aligns to `PlayerSpawnAnchor` above the operating table.
- Package Development and verify `Content/Localization/en.json` and `it.json` are staged.
- Validate room collision, door approach, note trace channel, exposure, and light performance.
