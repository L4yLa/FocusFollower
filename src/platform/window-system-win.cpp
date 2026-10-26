#include "window-system.hpp"

#include <plugin-support.h>
#include <util/base.h>

#include <windows.h>
#include <dwmapi.h>

#include <algorithm>
#include <iterator>
#include <map>

namespace ff {

namespace {

std::string toUtf8(const wchar_t *s, int len)
{
	if (len <= 0)
		return {};
	int n = WideCharToMultiByte(CP_UTF8, 0, s, len, nullptr, 0, nullptr, nullptr);
	std::string out(n, '\0');
	WideCharToMultiByte(CP_UTF8, 0, s, len, out.data(), n, nullptr, nullptr);
	return out;
}

std::string titleOf(HWND hwnd)
{
	wchar_t buf[512];
	int len = GetWindowTextW(hwnd, buf, (int)std::size(buf));
	return toUtf8(buf, len);
}

std::string exeOfPid(DWORD pid)
{
	HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!h)
		return {};
	wchar_t path[MAX_PATH];
	DWORD size = MAX_PATH;
	std::string exe;
	if (QueryFullProcessImageNameW(h, 0, path, &size)) {
		const wchar_t *name = path;
		for (DWORD i = 0; i < size; i++)
			if (path[i] == L'\\' || path[i] == L'/')
				name = path + i + 1;
		exe = toUtf8(name, (int)(path + size - name));
	}
	CloseHandle(h);
	return exe;
}

struct ChildSearch {
	DWORD framePid;
	DWORD found;
};

BOOL CALLBACK findUwpChild(HWND child, LPARAM param)
{
	auto *s = reinterpret_cast<ChildSearch *>(param);
	DWORD pid = 0;
	GetWindowThreadProcessId(child, &pid);
	if (pid && pid != s->framePid) {
		s->found = pid;
		return FALSE;
	}
	return TRUE;
}

// UWP apps are hosted by ApplicationFrameHost; the real process owns a child window.
std::string exeOf(HWND hwnd)
{
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (!pid)
		return {};
	std::string exe = exeOfPid(pid);
	if (lowerAscii(exe) == "applicationframehost.exe") {
		ChildSearch s{pid, 0};
		EnumChildWindows(hwnd, findUwpChild, reinterpret_cast<LPARAM>(&s));
		if (s.found) {
			std::string inner = exeOfPid(s.found);
			if (!inner.empty())
				exe = inner;
		}
	}
	return exe;
}

bool isAppWindow(HWND hwnd)
{
	if (!IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER))
		return false;
	if (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
		return false;
	DWORD cloaked = 0;
	if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked)
		return false;
	return GetWindowTextLengthW(hwnd) > 0;
}

BOOL CALLBACK collectApp(HWND hwnd, LPARAM param)
{
	if (!isAppWindow(hwnd))
		return TRUE;
	auto *apps = reinterpret_cast<std::map<std::string, RunningApp> *>(param);
	std::string exe = exeOf(hwnd);
	if (exe.empty())
		return TRUE;
	RunningApp &app = (*apps)[lowerAscii(exe)];
	if (app.exe.empty())
		app.exe = exe;
	app.titles.push_back(titleOf(hwnd));
	return TRUE;
}

WindowEvents *instance = nullptr;

void CALLBACK onWinEvent(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD)
{
	if (!instance || !hwnd)
		return;
	if (event != EVENT_SYSTEM_FOREGROUND && (idObject != OBJID_WINDOW || idChild != CHILDID_SELF))
		return;
	instance->notify();
}

} // namespace

WindowInfo foregroundWindow()
{
	static HWND cachedHwnd = nullptr;
	static DWORD cachedPid = 0;
	static std::string cachedExe;

	HWND hwnd = GetForegroundWindow();
	if (!hwnd)
		return {};

	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	bool frameHost = lowerAscii(cachedExe) == "applicationframehost.exe";
	if (hwnd != cachedHwnd || pid != cachedPid || cachedExe.empty() || frameHost) {
		cachedHwnd = hwnd;
		cachedPid = pid;
		cachedExe = exeOf(hwnd);
	}
	return WindowInfo{(uint64_t)(uintptr_t)hwnd, cachedExe, titleOf(hwnd)};
}

std::optional<std::string> windowTitle(uint64_t id)
{
	HWND hwnd = (HWND)(uintptr_t)id;
	if (!IsWindow(hwnd))
		return std::nullopt;
	return titleOf(hwnd);
}

std::vector<RunningApp> runningApps()
{
	std::map<std::string, RunningApp> apps;
	EnumWindows(collectApp, reinterpret_cast<LPARAM>(&apps));
	std::vector<RunningApp> out;
	for (auto &kv : apps)
		out.push_back(std::move(kv.second));
	return out;
}

WindowEvents::WindowEvents(std::function<void()> onChange) : onChange_(std::move(onChange))
{
	instance = this;
	const DWORD flags = WINEVENT_OUTOFCONTEXT;
	const DWORD events[] = {EVENT_SYSTEM_FOREGROUND, EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_DESTROY};
	for (DWORD e : events) {
		HWINEVENTHOOK h = SetWinEventHook(e, e, nullptr, onWinEvent, 0, 0, flags);
		if (h)
			hooks_.push_back(h);
	}
	obs_log(LOG_INFO, "window hooks: %zu/%zu", hooks_.size(), std::size(events));
}

WindowEvents::~WindowEvents()
{
	for (void *h : hooks_)
		UnhookWinEvent((HWINEVENTHOOK)h);
	if (instance == this)
		instance = nullptr;
}

void WindowEvents::notify()
{
	if (onChange_)
		onChange_();
}

} // namespace ff
