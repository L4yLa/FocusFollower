#include "window-system.hpp"

namespace ff {

WindowInfo foregroundWindow()
{
	return {};
}

std::optional<std::string> windowTitle(uint64_t)
{
	return std::nullopt;
}

std::vector<RunningApp> runningApps()
{
	return {};
}

HookStats takeHookStats()
{
	return {};
}

WindowEvents::WindowEvents(std::function<void()> onChange) : onChange_(std::move(onChange)) {}

WindowEvents::~WindowEvents() = default;

void WindowEvents::notify()
{
	if (onChange_)
		onChange_();
}

} // namespace ff
