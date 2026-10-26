#include "controller.hpp"

#include <obs-module.h>
#include <plugin-support.h>
#include <util/platform.h>

#include <QMetaObject>

#include <algorithm>

namespace {

const char *kConfigFile = "config.json";
constexpr int kMinWait = 0;
constexpr int kMaxWait = 60;

char *configPath()
{
	return obs_module_config_path(kConfigFile);
}

} // namespace

Controller::Controller()
{
	hotkey_ =
		obs_hotkey_register_frontend("focus_follower.toggle", obs_module_text("Hotkey.Toggle"), onHotkey, this);
}

Controller::~Controller()
{
	if (hotkey_ != OBS_INVALID_HOTKEY_ID)
		obs_hotkey_unregister(hotkey_);
}

void Controller::load()
{
	char *path = configPath();
	obs_data_t *data = path ? obs_data_create_from_json_file_safe(path, "bak") : nullptr;
	bfree(path);
	if (!data)
		return;

	obs_data_set_default_bool(data, "enabled", true);
	obs_data_set_default_int(data, "wait", 3);
	enabled_ = obs_data_get_bool(data, "enabled");
	setWaitSeconds((int)obs_data_get_int(data, "wait"));

	obs_data_array_t *keys = obs_data_get_array(data, "hotkey");
	if (keys) {
		obs_hotkey_load(hotkey_, keys);
		obs_data_array_release(keys);
	}
	obs_data_release(data);
}

void Controller::save() const
{
	char *dir = obs_module_config_path("");
	if (dir) {
		os_mkdirs(dir);
		bfree(dir);
	}

	obs_data_t *data = obs_data_create();
	obs_data_set_bool(data, "enabled", enabled_);
	obs_data_set_int(data, "wait", wait_);
	obs_data_array_t *keys = obs_hotkey_save(hotkey_);
	obs_data_set_array(data, "hotkey", keys);
	obs_data_array_release(keys);

	char *path = configPath();
	if (path && !obs_data_save_json_safe(data, path, "tmp", "bak"))
		obs_log(LOG_WARNING, "failed to save %s", path);
	bfree(path);
	obs_data_release(data);
}

void Controller::setEnabled(bool on)
{
	if (enabled_.exchange(on) == on)
		return;
	obs_log(LOG_INFO, "%s", on ? "enabled" : "disabled");
	emit enabledChanged(on);
}

void Controller::setWaitSeconds(int s)
{
	wait_ = std::clamp(s, kMinWait, kMaxWait);
}

void Controller::toggle()
{
	setEnabled(!enabled_);
}

void Controller::onHotkey(void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (!pressed)
		return;
	auto *self = static_cast<Controller *>(data);
	QMetaObject::invokeMethod(self, &Controller::toggle, Qt::QueuedConnection);
}
