#include "rules.hpp"

#include <algorithm>
#include <cctype>

namespace ff {

const QRegularExpression &TitleMatch::regex() const
{
	if (!hasCompiled_ || compiled_ != pattern) {
		re_ = QRegularExpression(QRegularExpression::anchoredPattern(QString::fromStdString(pattern)));
		compiled_ = pattern;
		hasCompiled_ = true;
	}
	return re_;
}

bool TitleMatch::valid() const
{
	if (mode == TitleMode::Regex)
		return regex().isValid();
	return true;
}

bool TitleMatch::matches(const std::string &title) const
{
	switch (mode) {
	case TitleMode::None:
		return true;
	case TitleMode::Exact:
		return title == pattern;
	case TitleMode::Partial:
		return title.find(pattern) != std::string::npos;
	case TitleMode::Regex: {
		const QRegularExpression &re = regex();
		return re.isValid() && re.match(QString::fromStdString(title)).hasMatch();
	}
	}
	return false;
}

std::string lowerAscii(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return s;
}

bool exeEquals(const std::string &a, const std::string &b)
{
	return !a.empty() && lowerAscii(a) == lowerAscii(b);
}

bool entryMatches(const SourceEntry &e, const WindowInfo &w)
{
	return exeEquals(e.exe, w.exe) && e.title.matches(w.title);
}

} // namespace ff
