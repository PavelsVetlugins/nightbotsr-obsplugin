# Volume Hotkeys Design Spec

## Goal

Add OBS hotkeys for increasing and decreasing Nightbot Song Request music volume, with a configurable step size in the plugin settings. Volume changes via hotkeys must persist between sessions using the existing persistent volume mechanism.

## Non-Goals

- Mute/unmute toggle hotkey (can be added later)
- Wrap-around volume (always clamp at boundaries)
- Hotkey-based step size adjustment (settings dialog only)

## Volume Range

- Overall volume: 0% to 100% (integer)
- Clamped at boundaries: volume up at 98% with 5% step results in 100%, volume down at 2% with 5% step results in 0%

## Hotkey Registration

Two new OBS frontend hotkeys in `src/plugin-main.cpp`, following the exact pattern of the existing pause/resume/skip hotkeys.

### New Global IDs

```cpp
static obs_hotkey_id g_nightbot_volume_up_hotkey_id;
static obs_hotkey_id g_nightbot_volume_down_hotkey_id;
```

### String ID Constants

```cpp
#define HOTKEY_VOLUME_UP_ID "nightbot_sr.volume_up"
#define HOTKEY_VOLUME_DOWN_ID "nightbot_sr.volume_down"
```

### Callbacks

`hotkey_volume_up(void*, obs_hotkey_id, obs_hotkey_t*, bool pressed)`:
- Only acts when `pressed == true`
- Reads current volume from `SettingsManager::get().GetVolume()`
- Reads step from `SettingsManager::get().GetVolumeStep()`
- Computes `newVolume = std::min(currentVolume + step, 100)`
- Calls `SettingsManager::get().SetVolumeWithFlag(newVolume)` (persists + sets user flag)
- Calls `NightbotAPI::get().SetVolume(newVolume)` (sends to Nightbot API)
- Updates dock slider via `QMetaObject::invokeMethod(g_dock_widget, [newVolume]() { ... }, Qt::QueuedConnection)`

`hotkey_volume_down`: same logic but `newVolume = std::max(currentVolume - step, 0)`.

### Registration

In `obs_module_load()`, after existing skip hotkey registration:

```cpp
g_nightbot_volume_up_hotkey_id = obs_hotkey_register_frontend(
    HOTKEY_VOLUME_UP_ID,
    obs_module_text("Nightbot.Hotkey.VolumeUp"),
    hotkey_volume_up, nullptr);

g_nightbot_volume_down_hotkey_id = obs_hotkey_register_frontend(
    HOTKEY_VOLUME_DOWN_ID,
    obs_module_text("Nightbot.Hotkey.VolumeDown"),
    hotkey_volume_down, nullptr);
```

Load saved keybindings from SettingsManager after registration (same pattern as existing hotkeys).

### Persistence

Add volume up/down hotkeys to the existing `save_hotkeys()` callback so their keybindings are saved/restored between sessions.

## Settings Persistence

### New Setting Key

In `src/SettingsManager.h`:

```cpp
namespace Setting {
    inline const char *VolumeStep = "volume_step";
}
```

### New Methods in SettingsManager

- `void SetVolumeStep(int step)` -- stores value via `obs_data_set_int()` and calls `Save()`
- `int GetVolumeStep()` -- returns stored value via `obs_data_get_int()`

### Default Value

In `SettingsManager::Load()`, if `volume_step` is not present, default to `5`:

```cpp
if (!obs_data_has_user_value(settings, Setting::VolumeStep))
    obs_data_set_int(settings, Setting::VolumeStep, 5);
```

## Settings Dialog

### New UI Group

A "Volume" group box added to `NightbotSettingsDialog` containing:

- `QSpinBox` for volume step
  - Range: 1-50
  - Default: 5
  - Suffix: "%"
  - Label: localized `"Volume Step (%)"`

### Behavior

On `valueChanged`, calls `SettingsManager::get().SetVolumeStep(value)` -- persisted immediately, matching the pattern of the refresh interval spin box.

## Dock Slider Sync

When a hotkey adjusts the volume, the dock slider must reflect the new value. The hotkey callback updates the slider via:

```cpp
QMetaObject::invokeMethod(g_dock_widget, [newVolume]() {
    g_dock_widget->updateVolumeSlider(newVolume);
}, Qt::QueuedConnection);
```

The existing `updateVolumeSlider(int)` method in `NightbotDock` already handles:
- `blockSignals(true/false)` around `setValue()` to prevent feedback loops
- `isSliderDown()` check to skip updates during user drag

This method needs to be made public so the hotkey callback can invoke it.

## Localization

New entries in `data/locale/en-US.ini`:

```ini
Nightbot.Hotkey.VolumeUp="Volume Up"
Nightbot.Hotkey.VolumeDown="Volume Down"
Nightbot.Settings.VolumeGroup="Volume"
Nightbot.Settings.VolumeStep="Volume Step (%)"
```

Corresponding entries in `pt-BR.ini` and `pt-PT.ini` with Portuguese translations.

## Files Modified

| File | Changes |
|------|---------|
| `src/plugin-main.cpp` | 2 new hotkey IDs, 2 callbacks, registration, save/load |
| `src/SettingsManager.h` | `VolumeStep` key, `GetVolumeStep()`/`SetVolumeStep()` declarations |
| `src/SettingsManager.cpp` | Getter/setter implementations, default in `Load()` |
| `src/nightbot-settings.h` | `QSpinBox *volumeStepSpinBox` member |
| `src/nightbot-settings.cpp` | Volume group box with spin box, `onVolumeStepChanged` handler |
| `src/nightbot-dock.h` | Make `updateVolumeSlider(int)` public |
| `data/locale/en-US.ini` | 4 new locale strings |
| `data/locale/pt-BR.ini` | 4 new locale strings (translated) |
| `data/locale/pt-PT.ini` | 4 new locale strings (translated) |

No new files created.
