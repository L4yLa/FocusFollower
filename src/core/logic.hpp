#pragma once

#include "rules.hpp"

#include <map>
#include <optional>

namespace ff {

// Fires once after the foreground stays unchanged for the wait time.
class Dwell {
public:
	void setWait(int64_t ns) { wait_ = ns; }
	void reset();
	void update(const WindowInfo &w, int64_t now);
	bool due(int64_t now);
	const WindowInfo &current() const { return cur_; }

private:
	int64_t wait_ = 3000000000LL;
	WindowInfo cur_;
	int64_t since_ = 0;
	bool known_ = false;
	bool fired_ = false;
};

struct Plan {
	int scene = -1;
	std::vector<int> entries; // per group, -1 if none

	bool empty() const;
};

// Valid flags come from the OBS side; invalid rules are ignored.
struct Validity {
	std::vector<bool> scenes;
	std::vector<bool> groups;
	std::vector<std::vector<bool>> entries;
};

Validity allValid(const Rules &r);
Plan evaluate(const Rules &r, const Validity &v, const WindowInfo &w);

struct Watch {
	uint64_t hwnd = 0;
	TitleMatch title;
};

// Sources shown by an entry with a title condition, keyed by source id.
class Watches {
public:
	void set(const std::string &source, uint64_t hwnd, const TitleMatch &t);
	void erase(const std::string &source) { map_.erase(source); }
	void clear() { map_.clear(); }
	bool empty() const { return map_.empty(); }

	// title is nullopt when the window no longer exists. Returns sources to hide.
	template<class TitleFn> std::vector<std::string> check(TitleFn &&titleOf);

private:
	std::map<std::string, Watch> map_;
};

template<class TitleFn> std::vector<std::string> Watches::check(TitleFn &&titleOf)
{
	std::vector<std::string> hide;
	for (auto it = map_.begin(); it != map_.end();) {
		std::optional<std::string> t = titleOf(it->second.hwnd);
		if (!t || !it->second.title.matches(*t)) {
			hide.push_back(it->first);
			it = map_.erase(it);
		} else {
			++it;
		}
	}
	return hide;
}

// Position for the item so it sits above the other members, or -1 if already on top.
int raisedPosition(int self, const std::vector<int> &others);

} // namespace ff
