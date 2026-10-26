#include "config.hpp"

#include <obs.hpp>

#include <algorithm>

namespace ff {

namespace {

std::string str(obs_data_t *d, const char *key)
{
	const char *s = obs_data_get_string(d, key);
	return s ? s : "";
}

template<class F> void forEach(obs_data_t *d, const char *key, F &&f)
{
	OBSDataArrayAutoRelease arr = obs_data_get_array(d, key);
	size_t n = obs_data_array_count(arr);
	for (size_t i = 0; i < n; i++) {
		OBSDataAutoRelease item = obs_data_array_item(arr, i);
		f(item.Get());
	}
}

void push(obs_data_array_t *arr, obs_data_t *item)
{
	obs_data_array_push_back(arr, item);
	obs_data_release(item);
}

} // namespace

obs_data_t *rulesToData(const Rules &rules)
{
	obs_data_t *data = obs_data_create();

	OBSDataArrayAutoRelease scenes = obs_data_array_create();
	for (const SceneRule &s : rules.scenes) {
		obs_data_t *d = obs_data_create();
		obs_data_set_string(d, "exe", s.exe.c_str());
		obs_data_set_string(d, "scene", s.scene.c_str());
		obs_data_set_string(d, "scene_name", s.sceneName.c_str());
		push(scenes, d);
	}
	obs_data_set_array(data, "scenes", scenes);

	OBSDataArrayAutoRelease groups = obs_data_array_create();
	for (const SourceGroup &g : rules.groups) {
		obs_data_t *gd = obs_data_create();
		obs_data_set_string(gd, "name", g.name.c_str());
		obs_data_set_int(gd, "mode", (int)g.mode);
		OBSDataArrayAutoRelease entries = obs_data_array_create();
		for (const SourceEntry &e : g.entries) {
			obs_data_t *d = obs_data_create();
			obs_data_set_string(d, "exe", e.exe.c_str());
			obs_data_set_int(d, "title_mode", (int)e.title.mode);
			obs_data_set_string(d, "title", e.title.pattern.c_str());
			obs_data_set_string(d, "source", e.source.c_str());
			obs_data_set_string(d, "source_name", e.sourceName.c_str());
			push(entries, d);
		}
		obs_data_set_array(gd, "entries", entries);
		push(groups, gd);
	}
	obs_data_set_array(data, "groups", groups);
	return data;
}

Rules rulesFromData(obs_data_t *data)
{
	Rules rules;
	if (!data)
		return rules;

	forEach(data, "scenes",
		[&](obs_data_t *d) { rules.scenes.push_back({str(d, "exe"), str(d, "scene"), str(d, "scene_name")}); });
	forEach(data, "groups", [&](obs_data_t *gd) {
		SourceGroup g;
		g.name = str(gd, "name");
		g.mode = obs_data_get_int(gd, "mode") == (int)GroupMode::ZOrder ? GroupMode::ZOrder
										: GroupMode::Exclusive;
		forEach(gd, "entries", [&](obs_data_t *d) {
			SourceEntry e;
			e.exe = str(d, "exe");
			e.title.mode = (TitleMode)std::clamp<long long>(obs_data_get_int(d, "title_mode"), 0, 3);
			e.title.pattern = str(d, "title");
			e.source = str(d, "source");
			e.sourceName = str(d, "source_name");
			g.entries.push_back(e);
		});
		rules.groups.push_back(g);
	});
	return rules;
}

} // namespace ff
