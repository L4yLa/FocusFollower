#include "validation.hpp"
#include "scene-ops.hpp"

#include <obs-module.h>

#include <set>

namespace ff {

Validity Report::validity() const
{
	Validity v;
	for (Problem p : scenes)
		v.scenes.push_back(p == Problem::None);
	for (Problem p : groups)
		v.groups.push_back(p == Problem::None);
	for (const auto &g : entries) {
		std::vector<bool> row;
		for (Problem p : g)
			row.push_back(p == Problem::None);
		v.entries.push_back(row);
	}
	return v;
}

Report checkRules(const Rules &rules)
{
	Report r;
	for (const SceneRule &s : rules.scenes) {
		OBSSourceAutoRelease src = sourceByUuid(s.scene);
		r.scenes.push_back(src && obs_scene_from_source(src) ? Problem::None : Problem::MissingScene);
	}

	std::set<std::string> earlierGroups;
	for (const SourceGroup &g : rules.groups) {
		std::vector<Problem> row;
		std::vector<bool> warn;
		std::set<std::string> mine;
		std::vector<OBSSource> members;

		for (const SourceEntry &e : g.entries) {
			OBSSourceAutoRelease src = sourceByUuid(e.source);
			Problem p = Problem::None;
			if (!src)
				p = Problem::MissingSource;
			else if (earlierGroups.count(e.source))
				p = Problem::Duplicate;
			else if (!e.title.valid())
				p = Problem::BadRegex;
			row.push_back(p);
			warn.push_back(src && isBrowserExe(e.exe) && capturePriorityWarning(src));

			if (src && mine.insert(e.source).second)
				members.emplace_back(src.Get());
		}

		Problem gp = Problem::None;
		if (g.mode == GroupMode::ZOrder) {
			std::vector<obs_source_t *> raw(members.begin(), members.end());
			if (!shareParent(raw))
				gp = Problem::ParentMismatch;
		}
		r.groups.push_back(gp);
		r.entries.push_back(row);
		r.priorityWarnings.push_back(warn);
		earlierGroups.insert(mine.begin(), mine.end());
	}
	return r;
}

const char *problemText(Problem p)
{
	switch (p) {
	case Problem::None:
		return "";
	case Problem::MissingScene:
		return obs_module_text("Problem.MissingScene");
	case Problem::MissingSource:
		return obs_module_text("Problem.MissingSource");
	case Problem::BadRegex:
		return obs_module_text("Problem.BadRegex");
	case Problem::ParentMismatch:
		return obs_module_text("Problem.ParentMismatch");
	case Problem::Duplicate:
		return obs_module_text("Problem.Duplicate");
	}
	return "";
}

} // namespace ff
