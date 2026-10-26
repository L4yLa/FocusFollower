#include "core/logic.hpp"

#include <cstdio>
#include <cstdlib>

using namespace ff;

static int failures = 0;

#define CHECK(cond)                                                       \
	do {                                                              \
		if (!(cond)) {                                            \
			std::printf("%s:%d: %s\n", __FILE__, __LINE__, #cond); \
			failures++;                                       \
		}                                                         \
	} while (0)

static TitleMatch tm(TitleMode m, const char *p)
{
	TitleMatch t;
	t.mode = m;
	t.pattern = p;
	return t;
}

static WindowInfo win(uint64_t h, const char *exe, const char *title)
{
	return WindowInfo{h, exe, title};
}

static void testTitle()
{
	CHECK(tm(TitleMode::None, "x").matches("anything"));
	CHECK(tm(TitleMode::Exact, "A - Chrome").matches("A - Chrome"));
	CHECK(!tm(TitleMode::Exact, "A - Chrome").matches("A - Chrome "));
	CHECK(tm(TitleMode::Partial, "配信").matches("今日の配信 - Chrome"));
	CHECK(!tm(TitleMode::Partial, "配信").matches("メモ"));
	CHECK(tm(TitleMode::Regex, ".* - Chrome").matches("今日 - Chrome"));
	CHECK(!tm(TitleMode::Regex, "Chrome").matches("今日 - Chrome"));
	CHECK(tm(TitleMode::Regex, "今.").matches("今日"));

	TitleMatch bad = tm(TitleMode::Regex, "(");
	CHECK(!bad.valid());
	CHECK(!bad.matches("("));
	CHECK(tm(TitleMode::Exact, "(").valid());

	TitleMatch t = tm(TitleMode::Regex, "a");
	CHECK(t.matches("a"));
	t.pattern = "b";
	CHECK(t.matches("b"));
	CHECK(!t.matches("a"));
}

static void testExe()
{
	CHECK(exeEquals("Chrome.EXE", "chrome.exe"));
	CHECK(!exeEquals("", ""));
	CHECK(!exeEquals("chrome.exe", "code.exe"));
}

static void testDwell()
{
	const int64_t s = 1000000000LL;
	Dwell d;
	d.setWait(3 * s);
	CHECK(!d.due(0));

	d.update(win(1, "a.exe", "x"), 0);
	CHECK(!d.due(2 * s));
	d.update(win(1, "a.exe", "x"), 2 * s);
	CHECK(d.due(3 * s));
	CHECK(!d.due(4 * s));

	d.update(win(1, "a.exe", "y"), 5 * s);
	CHECK(!d.due(7 * s));
	d.update(win(2, "b.exe", "y"), 7 * s);
	CHECK(!d.due(9 * s));
	CHECK(d.due(10 * s));

	d.reset();
	CHECK(!d.due(100 * s));
	d.update(win(2, "b.exe", "y"), 100 * s);
	CHECK(d.due(103 * s));
}

static Rules sampleRules()
{
	Rules r;
	r.scenes.push_back({"chrome.exe", "s1", "S1"});
	r.scenes.push_back({"chrome.exe", "s2", "S2"});
	r.scenes.push_back({"code.exe", "s3", "S3"});

	SourceGroup g;
	g.entries.push_back({"chrome.exe", tm(TitleMode::Exact, "A"), "src-a", "A"});
	g.entries.push_back({"chrome.exe", tm(TitleMode::None, ""), "src-b", "B"});
	r.groups.push_back(g);

	SourceGroup g2;
	g2.mode = GroupMode::ZOrder;
	g2.entries.push_back({"code.exe", tm(TitleMode::None, ""), "src-c", "C"});
	r.groups.push_back(g2);
	return r;
}

static void testEvaluate()
{
	Rules r = sampleRules();
	Validity v = allValid(r);

	Plan p = evaluate(r, v, win(1, "chrome.exe", "A"));
	CHECK(p.scene == 0);
	CHECK(p.entries.size() == 2);
	CHECK(p.entries[0] == 0);
	CHECK(p.entries[1] == -1);

	p = evaluate(r, v, win(1, "CHROME.exe", "B"));
	CHECK(p.scene == 0);
	CHECK(p.entries[0] == 1);

	p = evaluate(r, v, win(1, "code.exe", "x"));
	CHECK(p.scene == 2);
	CHECK(p.entries[0] == -1);
	CHECK(p.entries[1] == 0);

	p = evaluate(r, v, win(1, "obs64.exe", "OBS"));
	CHECK(p.empty());

	v.scenes[0] = false;
	v.entries[0][0] = false;
	p = evaluate(r, v, win(1, "chrome.exe", "A"));
	CHECK(p.scene == 1);
	CHECK(p.entries[0] == 1);

	v.groups[0] = false;
	p = evaluate(r, v, win(1, "chrome.exe", "A"));
	CHECK(p.entries[0] == -1);
}

static void testWatches()
{
	Watches w;
	w.set("a", 1, tm(TitleMode::Exact, "A"));
	w.set("b", 2, tm(TitleMode::Partial, "B"));
	w.set("c", 3, tm(TitleMode::Exact, "C"));

	auto titles = [](uint64_t h) -> std::optional<std::string> {
		if (h == 1)
			return std::string("A");
		if (h == 2)
			return std::string("xx");
		return std::nullopt;
	};
	std::vector<std::string> hide = w.check(titles);
	CHECK(hide.size() == 2);
	CHECK(hide[0] == "b");
	CHECK(hide[1] == "c");
	CHECK(w.check(titles).empty());

	w.erase("a");
	CHECK(w.empty());
}

static void testRaise()
{
	CHECK(raisedPosition(0, {1, 2}) == 2);
	CHECK(raisedPosition(3, {1, 2}) == -1);
	CHECK(raisedPosition(1, {}) == -1);
}

int main()
{
	testTitle();
	testExe();
	testDwell();
	testEvaluate();
	testWatches();
	testRaise();
	if (failures)
		std::printf("%d failure(s)\n", failures);
	else
		std::printf("all passed\n");
	return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
