#pragma once

#include <obs.hpp>

#include <string>
#include <vector>

namespace ff {

OBSSourceAutoRelease sourceByUuid(const std::string &uuid);
std::vector<OBSSceneItem> childItems(obs_scene_t *scene);

// Items of target reachable from front through visible parents. Target's own visibility is ignored.
std::vector<OBSSceneItem> onScreenItems(obs_source_t *front, obs_source_t *target);

// Hides every item of target in all scenes and groups of all canvases.
void hideEverywhere(obs_source_t *target);

// Scenes and groups of all canvases.
std::vector<OBSSource> allParents();

// True if some scene or group directly contains all sources.
bool shareParent(const std::vector<obs_source_t *> &sources);

// Window or game capture that may fall back to another window.
bool capturePriorityWarning(obs_source_t *source);

} // namespace ff
