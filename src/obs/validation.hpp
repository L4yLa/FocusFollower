#pragma once

#include "core/logic.hpp"

namespace ff {

enum class Problem { None, MissingScene, MissingSource, BadRegex, ParentMismatch, Duplicate };

struct Report {
	std::vector<Problem> scenes;
	std::vector<Problem> groups;
	std::vector<std::vector<Problem>> entries;
	std::vector<std::vector<bool>> priorityWarnings;

	Validity validity() const;
};

Report checkRules(const Rules &rules);
const char *problemText(Problem p);

} // namespace ff
