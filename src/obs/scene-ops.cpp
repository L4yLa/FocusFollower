#include "scene-ops.hpp"

#include <algorithm>
#include <cstring>

namespace ff {

namespace {

constexpr int kMaxDepth = 16;
constexpr long long kPriorityTitle = 1;

obs_scene_t *innerScene(obs_sceneitem_t *item)
{
	if (obs_sceneitem_is_group(item))
		return obs_sceneitem_group_get_scene(item);
	return obs_scene_from_source(obs_sceneitem_get_source(item));
}

void collectOnScreen(obs_scene_t *scene, obs_source_t *target, int depth, std::vector<OBSSceneItem> &out)
{
	if (!scene || depth > kMaxDepth)
		return;
	for (OBSSceneItem &item : childItems(scene)) {
		if (obs_sceneitem_get_source(item) == target) {
			out.push_back(item);
			continue;
		}
		if (obs_sceneitem_visible(item))
			collectOnScreen(innerScene(item), target, depth + 1, out);
	}
}

void addWithGroups(obs_source_t *scene, std::vector<OBSSource> &out)
{
	out.emplace_back(scene);
	for (OBSSceneItem &item : childItems(obs_group_or_scene_from_source(scene)))
		if (obs_sceneitem_is_group(item))
			addWithGroups(obs_sceneitem_get_source(item), out);
}

} // namespace

OBSSourceAutoRelease sourceByUuid(const std::string &uuid)
{
	if (uuid.empty())
		return nullptr;
	return obs_get_source_by_uuid(uuid.c_str());
}

std::vector<OBSSceneItem> childItems(obs_scene_t *scene)
{
	std::vector<OBSSceneItem> items;
	if (!scene)
		return items;
	obs_scene_enum_items(
		scene,
		[](obs_scene_t *, obs_sceneitem_t *item, void *param) {
			static_cast<std::vector<OBSSceneItem> *>(param)->emplace_back(item);
			return true;
		},
		&items);
	return items;
}

std::vector<OBSSceneItem> onScreenItems(obs_source_t *front, obs_source_t *target)
{
	std::vector<OBSSceneItem> out;
	if (front && target)
		collectOnScreen(obs_scene_from_source(front), target, 0, out);
	return out;
}

std::vector<OBSSource> allParents()
{
	std::vector<OBSSource> scenes;
	obs_enum_canvases(
		[](void *param, obs_canvas_t *canvas) {
			obs_canvas_enum_scenes(
				canvas,
				[](void *p, obs_source_t *scene) {
					static_cast<std::vector<OBSSource> *>(p)->emplace_back(scene);
					return true;
				},
				param);
			return true;
		},
		&scenes);

	std::vector<OBSSource> out;
	for (OBSSource &s : scenes)
		addWithGroups(s, out);
	return out;
}

void hideEverywhere(obs_source_t *target)
{
	if (!target)
		return;
	for (OBSSource &parent : allParents())
		for (OBSSceneItem &item : childItems(obs_group_or_scene_from_source(parent)))
			if (obs_sceneitem_get_source(item) == target && obs_sceneitem_visible(item))
				obs_sceneitem_set_visible(item, false);
}

bool shareParent(const std::vector<obs_source_t *> &sources)
{
	if (sources.size() < 2)
		return true;
	for (OBSSource &parent : allParents()) {
		std::vector<obs_source_t *> children;
		for (OBSSceneItem &item : childItems(obs_group_or_scene_from_source(parent)))
			children.push_back(obs_sceneitem_get_source(item));
		bool all = std::all_of(sources.begin(), sources.end(), [&](obs_source_t *s) {
			return std::find(children.begin(), children.end(), s) != children.end();
		});
		if (all)
			return true;
	}
	return false;
}

bool capturePriorityWarning(obs_source_t *source)
{
	if (!source)
		return false;
	const char *id = obs_source_get_unversioned_id(source);
	bool window = id && strcmp(id, "window_capture") == 0;
	bool game = id && strcmp(id, "game_capture") == 0;
	if (!window && !game)
		return false;

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	if (game && strcmp(obs_data_get_string(settings, "capture_mode"), "window") != 0)
		return false;
	return obs_data_get_int(settings, "priority") != kPriorityTitle;
}

} // namespace ff
