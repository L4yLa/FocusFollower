#include "engine.hpp"
#include "controller.hpp"
#include "obs/scene-ops.hpp"
#include "obs/validation.hpp"

#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <util/platform.h>

#include <set>

using namespace ff;

namespace {

constexpr int kPollMs = 100;

} // namespace

Engine::Engine(Controller *controller) : controller_(controller)
{
	connect(&poll_, &QTimer::timeout, this, &Engine::tick);
	connect(controller_, &Controller::enabledChanged, this, &Engine::onEnabledChanged);
	poll_.start(kPollMs);
	events_ = std::make_unique<WindowEvents>([this]() { schedule(); });
}

Engine::~Engine() = default;

void Engine::setRules(const Rules &rules)
{
	rules_ = rules;
	dwell_.reset();
	size_t entries = 0;
	for (const SourceGroup &g : rules_.groups)
		entries += g.entries.size();
	obs_log(LOG_INFO, "rules: %zu scene, %zu group, %zu source", rules_.scenes.size(), rules_.groups.size(),
		entries);

	Report report = checkRules(rules_);
	for (size_t i = 0; i < report.scenes.size(); i++)
		if (report.scenes[i] != Problem::None)
			obs_log(LOG_INFO, "scene rule %zu disabled: %d", i + 1, (int)report.scenes[i]);
	for (size_t g = 0; g < report.groups.size(); g++) {
		if (report.groups[g] != Problem::None)
			obs_log(LOG_INFO, "group %zu disabled: %d", g + 1, (int)report.groups[g]);
		for (size_t e = 0; e < report.entries[g].size(); e++)
			if (report.entries[g][e] != Problem::None)
				obs_log(LOG_INFO, "group %zu rule %zu disabled: %d", g + 1, e + 1,
					(int)report.entries[g][e]);
	}
}

void Engine::clearState()
{
	dwell_.reset();
	watches_.clear();
}

void Engine::reset()
{
	rules_ = {};
	clearState();
}

void Engine::stop()
{
	poll_.stop();
	events_.reset();
	clearState();
}

void Engine::onEnabledChanged(bool)
{
	clearState();
}

void Engine::schedule()
{
	if (scheduled_)
		return;
	scheduled_ = true;
	QTimer::singleShot(0, this, [this]() {
		scheduled_ = false;
		tick();
	});
}

void Engine::tick()
{
	if (!poll_.isActive() || !controller_->enabled())
		return;

	if (!watches_.empty()) {
		for (const std::string &uuid : watches_.check(windowTitle)) {
			OBSSourceAutoRelease src = sourceByUuid(uuid);
			hideEverywhere(src);
		}
	}

	int64_t now = (int64_t)os_gettime_ns();
	dwell_.setWait((int64_t)controller_->waitSeconds() * 1000000000LL);
	dwell_.update(foregroundWindow(), now);
	if (dwell_.due(now))
		apply(dwell_.current());
}

void Engine::apply(const WindowInfo &w)
{
	if (w.hwnd == 0)
		return;
	Report report = checkRules(rules_);
	Plan plan = evaluate(rules_, report.validity(), w);
	obs_log(LOG_INFO, "focus: %s \"%s\" scene=%d", w.exe.c_str(), w.title.c_str(), plan.scene);
	if (plan.empty())
		return;

	OBSSourceAutoRelease current = obs_frontend_get_current_scene();
	OBSSource front = current.Get();

	if (plan.scene >= 0) {
		OBSSourceAutoRelease target = sourceByUuid(rules_.scenes[plan.scene].scene);
		if (target) {
			if (target.Get() != front.Get())
				obs_frontend_set_current_scene(target);
			front = target.Get();
		}
	}

	for (size_t g = 0; g < plan.entries.size(); g++) {
		int e = plan.entries[g];
		if (e >= 0)
			applyGroup(front, rules_.groups[g], rules_.groups[g].entries[e], w.hwnd);
	}
}

void Engine::applyGroup(obs_source_t *front, const SourceGroup &group, const SourceEntry &entry, uint64_t hwnd)
{
	OBSSourceAutoRelease src = sourceByUuid(entry.source);
	std::vector<OBSSceneItem> items = onScreenItems(front, src);
	obs_log(LOG_INFO, "group \"%s\": %s on screen %zu in \"%s\"", group.name.c_str(), entry.sourceName.c_str(),
		items.size(), front ? obs_source_get_name(front) : "");
	if (items.empty())
		return;

	std::set<std::string> others;
	for (const SourceEntry &o : group.entries)
		if (o.source != entry.source)
			others.insert(o.source);

	if (group.mode == GroupMode::Exclusive) {
		for (const std::string &uuid : others) {
			OBSSourceAutoRelease other = sourceByUuid(uuid);
			for (OBSSceneItem &item : onScreenItems(front, other))
				obs_sceneitem_set_visible(item, false);
			watches_.erase(uuid);
		}
	} else {
		std::set<obs_source_t *> otherSources;
		for (const std::string &uuid : others) {
			OBSSourceAutoRelease other = sourceByUuid(uuid);
			if (other)
				otherSources.insert(other.Get());
		}
		for (OBSSceneItem &item : items) {
			std::vector<int> positions;
			for (OBSSceneItem &sibling : childItems(obs_sceneitem_get_scene(item)))
				if (otherSources.count(obs_sceneitem_get_source(sibling)))
					positions.push_back(obs_sceneitem_get_order_position(sibling));
			int pos = raisedPosition(obs_sceneitem_get_order_position(item), positions);
			if (pos >= 0)
				obs_sceneitem_set_order_position(item, pos);
		}
	}

	for (OBSSceneItem &item : items)
		obs_sceneitem_set_visible(item, true);

	if (entry.title.mode != TitleMode::None)
		watches_.set(entry.source, hwnd, entry.title);
	else
		watches_.erase(entry.source);
}
