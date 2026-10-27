#pragma once

#include <QRegularExpression>

#include <cstdint>
#include <string>
#include <vector>

namespace ff {

enum class TitleMode { None = 0, Exact = 1, Partial = 2, Regex = 3 };

struct TitleMatch {
	TitleMode mode = TitleMode::None;
	std::string pattern;

	bool valid() const;
	bool matches(const std::string &title) const;

private:
	mutable QRegularExpression re_;
	mutable std::string compiled_;
	mutable bool hasCompiled_ = false;
	const QRegularExpression &regex() const;
};

struct SceneRule {
	std::string exe;
	std::string scene;
	std::string sceneName;
};

struct SourceEntry {
	std::string exe;
	TitleMatch title;
	std::string source;
	std::string sourceName;
};

enum class GroupMode { Exclusive = 0, ZOrder = 1 };

struct SourceGroup {
	std::string name;
	GroupMode mode = GroupMode::Exclusive;
	std::vector<SourceEntry> entries;
};

struct Rules {
	std::vector<SceneRule> scenes;
	std::vector<SourceGroup> groups;
};

struct WindowInfo {
	uint64_t hwnd = 0;
	std::string exe;
	std::string title;

	bool operator==(const WindowInfo &o) const { return hwnd == o.hwnd && exe == o.exe && title == o.title; }
	bool operator!=(const WindowInfo &o) const { return !(*this == o); }
};

std::string lowerAscii(std::string s);
bool exeEquals(const std::string &a, const std::string &b);
bool entryMatches(const SourceEntry &e, const WindowInfo &w);
bool isBrowserExe(const std::string &exe);

} // namespace ff
