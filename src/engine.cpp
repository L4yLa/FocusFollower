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
}

void Engine::reset()
{
	rules_ = {};
	dwell_.reset();
	watches_.clear();
}

void Engine::onEnabledChanged(bool)
{
	dwell_.reset();
	watches_.clear();
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
	if (!controller_->enabled())
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
