#include "logic.hpp"

#include <algorithm>

namespace ff {

void Dwell::reset()
{
	cur_ = {};
	known_ = false;
	fired_ = false;
}

void Dwell::update(const WindowInfo &w, int64_t now)
{
	if (known_ && w == cur_)
		return;
	cur_ = w;
	since_ = now;
	known_ = true;
	fired_ = false;
}

bool Dwell::due(int64_t now)
{
	if (!known_ || fired_ || now - since_ < wait_)
		return false;
	fired_ = true;
	return true;
}

bool Plan::empty() const
{
	if (scene >= 0)
		return false;
	for (int e : entries)
		if (e >= 0)
			return false;
	return true;
}

Validity allValid(const Rules &r)
{
	Validity v;
	v.scenes.assign(r.scenes.size(), true);
	v.groups.assign(r.groups.size(), true);
	for (const SourceGroup &g : r.groups)
		v.entries.emplace_back(g.entries.size(), true);
	return v;
}

Plan evaluate(const Rules &r, const Validity &v, const WindowInfo &w)
{
	Plan p;
	for (size_t i = 0; i < r.scenes.size(); i++) {
		if (v.scenes[i] && exeEquals(r.scenes[i].exe, w.exe)) {
			p.scene = (int)i;
			break;
		}
	}

	p.entries.assign(r.groups.size(), -1);
	for (size_t g = 0; g < r.groups.size(); g++) {
		if (!v.groups[g])
			continue;
		const auto &entries = r.groups[g].entries;
		for (size_t e = 0; e < entries.size(); e++) {
			if (v.entries[g][e] && entryMatches(entries[e], w)) {
				p.entries[g] = (int)e;
				break;
			}
		}
	}
	return p;
}

void Watches::set(const std::string &source, uint64_t hwnd, const TitleMatch &t)
{
	map_[source] = Watch{hwnd, t};
}

int raisedPosition(int self, const std::vector<int> &others)
{
	int top = -1;
	for (int o : others)
		top = std::max(top, o);
	return top > self ? top : -1;
}

} // namespace ff
