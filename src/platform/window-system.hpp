#pragma once

#include "core/rules.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ff {

struct RunningApp {
	std::string exe;
	std::vector<std::string> titles;
};

WindowInfo foregroundWindow();
std::optional<std::string> windowTitle(uint64_t hwnd);
std::vector<RunningApp> runningApps();

// Temporary load measurement.
struct HookStats {
	uint64_t foreground = 0;
	uint64_t nameChange = 0;
	uint64_t destroy = 0;
	uint64_t passed = 0;
	uint64_t callbackNs = 0;
};
HookStats takeHookStats();

// Notifies on foreground change, title change and window destruction.
class WindowEvents {
public:
	explicit WindowEvents(std::function<void()> onChange);
	~WindowEvents();
	WindowEvents(const WindowEvents &) = delete;
	WindowEvents &operator=(const WindowEvents &) = delete;

	void notify();

private:
	std::function<void()> onChange_;
	std::vector<void *> hooks_;
};

} // namespace ff
