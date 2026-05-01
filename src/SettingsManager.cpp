#include "SettingsManager.h"
#include <obs-module.h>
#include "plugin-support.h"

static const char *SETTINGS_FILE_NAME = "settings.json";

static SettingsManager *s_instance = nullptr;

SettingsManager &SettingsManager::get()
{
	if (!s_instance)
		s_instance = new SettingsManager();
	return *s_instance;
}

void FreeSettingsManager()
{
	if (s_instance) {
		delete s_instance;
		s_instance = nullptr;
	}
}

SettingsManager::~SettingsManager()
{
	if (settings) {
		Save();
		obs_data_release(settings);
		settings = nullptr;
	}
}

void SettingsManager::Load()
{
    this->settings = obs_data_create_from_json_file(obs_module_config_path(SETTINGS_FILE_NAME));

	if (!settings) {
        obs_log_info("[Nightbot SR/Settings] No config found. Creating new one...");
		settings = obs_data_create();

        obs_data_set_string(settings, Setting::AccessToken, "");
        obs_data_set_string(settings, Setting::RefreshToken, "");
        obs_data_set_string(settings, Setting::UserName, "");
		obs_data_set_bool(settings, Setting::AutoRefreshEnabled, true);
		obs_data_set_int(settings, Setting::AutoRefreshInterval, 5);
		obs_data_set_string(settings, Setting::NowPlayingSource, "");
		obs_data_set_string(settings, Setting::NowPlayingFormat, "Now Playing: {music} - {artist}");
		obs_data_set_bool(settings, Setting::NowPlayingToFileEnabled, false);
		obs_data_set_string(settings, Setting::NowPlayingToFilePath, "");
	}

	// Apply defaults for keys that may be missing after an upgrade.
	// obs_data_get_int returns 0 for missing keys, so Volume needs an
	// explicit default to avoid silencing playback on first upgrade.
	if (!obs_data_has_user_value(settings, Setting::Volume))
		obs_data_set_int(settings, Setting::Volume, 100);
	if (!obs_data_has_user_value(settings, Setting::VolumeStep))
		obs_data_set_int(settings, Setting::VolumeStep, 5);
}

void SettingsManager::Save()
{
	if (!settings){
        obs_log_error("[Nightbot SR/Settings] Attempt to save null config.");
		return;
    }

    const char *config_path_c = obs_module_config_path(SETTINGS_FILE_NAME);
	if (!config_path_c) {
		obs_log_error("[Nightbot SR/Settings] Could not get config path for saving.");
		return;
	}

    QString path = QString::fromUtf8(config_path_c);
	QFileInfo info(path);
	QDir dir = info.dir();

	if (!dir.exists())
		dir.mkpath(".");

	if (obs_data_save_json(settings, config_path_c)) {
		obs_log_info("[Nightbot SR/Settings] Config saved to: %s", config_path_c);
	} else {
		obs_log_warning("[Nightbot SR/Settings] Failed to save config to: %s", config_path_c);
	}
}

void SettingsManager::SetAccessToken(const std::string &token)
{
	obs_data_set_string(settings, Setting::AccessToken, token.c_str());
}

std::string SettingsManager::GetAccessToken()
{
	if (!settings)
		return "";

	const char *value = obs_data_get_string(settings, Setting::AccessToken);
	return (value) ? value : "";
}

void SettingsManager::SetRefreshToken(const std::string &token)
{
	obs_data_set_string(settings, Setting::RefreshToken, token.c_str());
}

std::string SettingsManager::GetRefreshToken()
{
	if (!settings)
		return "";

	const char *value = obs_data_get_string(settings, Setting::RefreshToken);
	return (value) ? value : "";
}

void SettingsManager::SetUserName(const std::string &name)
{
	obs_data_set_string(settings, Setting::UserName, name.c_str());
	Save();
}

std::string SettingsManager::GetNightUserName()
{
	if (!settings)
		return "";

	const char *value = obs_data_get_string(settings, Setting::UserName);
	return (value) ? value : "";
}

bool SettingsManager::GetAutoRefreshEnabled()
{
	return obs_data_get_bool(settings, Setting::AutoRefreshEnabled);
}

void SettingsManager::SetAutoRefreshInterval(int interval)
{
	obs_data_set_int(settings, Setting::AutoRefreshInterval, interval);
	Save();
}

int SettingsManager::GetAutoRefreshInterval()
{
	return static_cast<int>(obs_data_get_int(settings, Setting::AutoRefreshInterval));
}

void SettingsManager::SetVolume(int volume)
{
	obs_data_set_int(settings, Setting::Volume, volume);
	Save();
}

void SettingsManager::SetVolumeWithFlag(int volume)
{
	obs_data_set_int(settings, Setting::Volume, volume);
	obs_data_set_bool(settings, Setting::VolumeUserSet, true);
	Save();
}

int SettingsManager::GetVolume()
{
	return static_cast<int>(obs_data_get_int(settings, Setting::Volume));
}

void SettingsManager::SetVolumeUserSet(bool userSet)
{
	obs_data_set_bool(settings, Setting::VolumeUserSet, userSet);
	Save();
}

bool SettingsManager::GetVolumeUserSet()
{
	return obs_data_get_bool(settings, Setting::VolumeUserSet);
}

void SettingsManager::SetVolumeStep(int step)
{
	obs_data_set_int(settings, Setting::VolumeStep, step);
	Save();
}

int SettingsManager::GetVolumeStep()
{
	return static_cast<int>(obs_data_get_int(settings, Setting::VolumeStep));
}

void SettingsManager::SetAutoRefreshEnabled(bool enabled)
{
	obs_data_set_bool(settings, Setting::AutoRefreshEnabled, enabled);
	Save();
}

void SettingsManager::SetNowPlayingSource(const std::string &sourceName)
{
	obs_data_set_string(settings, Setting::NowPlayingSource, sourceName.c_str());
	Save();
}

std::string SettingsManager::GetNowPlayingSource()
{
	const char *value =
		obs_data_get_string(settings, Setting::NowPlayingSource);
	return (value) ? value : "";
}

void SettingsManager::SetNowPlayingFormat(const std::string &format)
{
	obs_data_set_string(settings, Setting::NowPlayingFormat, format.c_str());
	Save();
}

std::string SettingsManager::GetNowPlayingFormat()
{
	const char *value =
		obs_data_get_string(settings, Setting::NowPlayingFormat);
	return (value && *value) ? value : "Now Playing: {music} - {artist}";
}

void SettingsManager::SetNowPlayingToFileEnabled(bool enabled)
{
	obs_data_set_bool(settings, Setting::NowPlayingToFileEnabled, enabled);
	Save();
}

bool SettingsManager::GetNowPlayingToFileEnabled()
{
	return obs_data_get_bool(settings, Setting::NowPlayingToFileEnabled);
}

void SettingsManager::SetNowPlayingToFilePath(const std::string &path)
{
	obs_data_set_string(settings, Setting::NowPlayingToFilePath, path.c_str());
	Save();
}

std::string SettingsManager::GetNowPlayingToFilePath()
{
	const char *value = obs_data_get_string(settings, Setting::NowPlayingToFilePath);
	return (value) ? value : "";
}

obs_data_array_t *SettingsManager::GetHotkeyData(const char *key) const
{
	return obs_data_get_array(settings, key);
}

void SettingsManager::SetHotkeyData(const char *key, obs_data_array_t *hotkeyArray)
{
	obs_data_set_array(settings, key, hotkeyArray);
}
