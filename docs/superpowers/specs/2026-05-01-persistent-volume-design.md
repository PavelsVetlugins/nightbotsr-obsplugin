# Persistent Volume

## Problem

When the Nightbot song queue empties (last song ends, or on initial load with no queue), the Nightbot web backend resets volume to 100%. The OBS plugin picks up this reset via `FetchSRSettings()` and updates the slider, losing the user's preferred volume. The next song that plays starts at 100% instead of the user's chosen level.

## Goal

The plugin should remember the user's volume preference locally and enforce it on the Nightbot API, so playback always uses the stored volume regardless of server-side resets.

## Design

### SettingsManager: Store Volume Locally

Add a `Volume` setting key with default value 100 (matches Nightbot's default for new installs).

Add `SetVolume(int)` and `GetVolume()` methods following the existing pattern: set the value in `obs_data_t` and call `Save()`.

A boolean flag `VolumeUserSet` (default `false`) tracks whether the user has ever explicitly set volume. This distinguishes "user never touched volume, accept API value" from "user set volume, enforce it against resets."

### NightbotDock: Store on User Change

In `onVolumeChanged()` (triggered when the user releases the slider):
1. Save the new volume to `SettingsManager::SetVolume()`
2. Set `VolumeUserSet` to `true` via `SettingsManager::SetVolumeUserSet(true)`
3. Push to API via `NightbotAPI::SetVolume()` (existing behavior)

### NightbotDock: Detect and Correct Resets

In `updateVolumeSlider()` (called when `volumeFetched` signal arrives from API):
1. If the user is dragging the slider, skip (existing behavior)
2. If `VolumeUserSet` is `false`, accept the API value as-is and update the slider (existing behavior for new installs)
3. If `VolumeUserSet` is `true` and the API volume differs from the stored volume:
   - Push the stored volume back to the API via `NightbotAPI::SetVolume()`
   - Update the slider to the stored value (not the API's reset value)
4. If `VolumeUserSet` is `true` and the API volume matches the stored value, update the slider normally

### Startup Flow

No separate startup logic is needed. The existing flow handles it:
1. Plugin loads, `SettingsManager::Load()` restores settings including volume
2. If authenticated, `FetchSRSettings()` fires
3. `volumeFetched` signal triggers `updateVolumeSlider()`
4. The detection logic in step 3 above catches any mismatch and pushes the stored volume to the API

### Files Changed

- `src/SettingsManager.h` -- Add `Volume` and `VolumeUserSet` setting keys, getter/setter declarations
- `src/SettingsManager.cpp` -- Implement getters/setters, add defaults in `Load()`
- `src/nightbot-dock.cpp` -- Modify `onVolumeChanged()` to persist, modify `updateVolumeSlider()` to detect and correct resets

### Edge Cases

- **First install:** `VolumeUserSet` is `false`, volume default is 100. The API value is accepted. The user's first slider change stores their preference and sets the flag.
- **User intentionally sets 100%:** Works correctly. `VolumeUserSet` becomes `true`, stored volume is 100. API returning 100 after a reset matches, so no correction fires. If the user later changes to 50% and the API resets to 100, the mismatch is detected and corrected.
- **API call failure on correction:** `NightbotAPI::SetVolume()` is fire-and-forget (no success/failure callback). The next `FetchSRSettings()` poll will detect the mismatch again and retry the correction automatically.
- **User drags slider during a fetch:** The `isSliderDown()` guard prevents overwriting the user's in-progress interaction. The stored value updates when they release.

## Non-Goals

- Changing the volume slider UI or adding new UI elements
- Adding success/failure feedback for the `SetVolume` API call
- Syncing volume across multiple OBS instances
