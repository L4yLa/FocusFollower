/*
Focus Follower
Copyright (C) 2026 L4yLa

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "controller.hpp"
#include "dock.hpp"
#include "engine.hpp"
#include "obs/config.hpp"
#include "ui/settings-dialog.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <plugin-support.h>

#include <QMainWindow>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

namespace {

const char *kSaveKey = "focus-follower";

Controller *controller = nullptr;
Engine *engine = nullptr;

void onFrontendEvent(enum obs_frontend_event event, void *)
{
	switch (event) {
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING:
		// The old collection is saved after this event, so keep the rules.
		if (engine)
			engine->clearState();
		break;
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP:
		if (engine)
			engine->reset();
		break;
	case OBS_FRONTEND_EVENT_EXIT:
		if (engine)
			engine->stop();
		if (controller)
			controller->save();
		break;
	default:
		break;
	}
}

// Rules are stored in the scene collection.
void onSave(obs_data_t *data, bool saving, void *)
{
	if (!engine)
		return;
	if (saving) {
		OBSDataAutoRelease obj = ff::rulesToData(engine->rules());
		obs_data_set_obj(data, kSaveKey, obj);
	} else {
		OBSDataAutoRelease obj = obs_data_get_obj(data, kSaveKey);
		engine->setRules(ff::rulesFromData(obj));
	}
}

void openSettings(void *)
{
	if (!engine || !controller)
		return;
	auto *parent = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	SettingsDialog dlg(engine, controller, parent);
	dlg.exec();
}

} // namespace

bool obs_module_load(void)
{
	controller = new Controller();
	controller->load();
	engine = new Engine(controller);

	auto *dock = new Dock(controller);
	if (!obs_frontend_add_dock_by_id("focus-follower", obs_module_text("FocusFollower"), dock))
		obs_log(LOG_WARNING, "failed to add dock");

	obs_frontend_add_tools_menu_item(obs_module_text("ToolsMenu"), openSettings, nullptr);
	obs_frontend_add_event_callback(onFrontendEvent, nullptr);
	obs_frontend_add_save_callback(onSave, nullptr);
	obs_log(LOG_INFO, "loaded (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_save_callback(onSave, nullptr);
	obs_frontend_remove_event_callback(onFrontendEvent, nullptr);
	delete engine;
	engine = nullptr;
	delete controller;
	controller = nullptr;
}
