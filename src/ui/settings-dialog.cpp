#include "settings-dialog.hpp"
#include "controller.hpp"
#include "engine.hpp"
#include "obs/scene-ops.hpp"
#include "obs/validation.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <set>

using namespace ff;

namespace {

QString qs(const std::string &s)
{
	return QString::fromStdString(s);
}

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

QTreeWidget *makeList(const QStringList &headers, QWidget *parent)
{
	auto *list = new QTreeWidget(parent);
	list->setHeaderLabels(headers);
	list->setRootIsDecorated(false);
	list->setSelectionMode(QAbstractItemView::SingleSelection);
	list->setDragDropMode(QAbstractItemView::InternalMove);
	list->setDefaultDropAction(Qt::MoveAction);
	list->setAllColumnsShowFocus(true);
	list->header()->setStretchLastSection(true);
	return list;
}

QTreeWidgetItem *makeItem(int index, const QStringList &cols)
{
	auto *item = new QTreeWidgetItem(cols);
	item->setData(0, Qt::UserRole, index);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
	return item;
}

QPushButton *button(const char *key, QWidget *parent)
{
	return new QPushButton(text(key), parent);
}

// Current name from OBS, falling back to the stored one.
std::string liveName(const std::string &uuid, std::string &stored)
{
	OBSSourceAutoRelease src = sourceByUuid(uuid);
	if (src)
		stored = obs_source_get_name(src);
	return stored;
}

QString titleModeText(TitleMode m)
{
	switch (m) {
	case TitleMode::Exact:
		return text("TitleMode.Exact");
	case TitleMode::Partial:
		return text("TitleMode.Partial");
	case TitleMode::Regex:
		return text("TitleMode.Regex");
	default:
		return {};
	}
}

int selectedIndex(QTreeWidget *list)
{
	QTreeWidgetItem *item = list->currentItem();
	return item ? item->data(0, Qt::UserRole).toInt() : -1;
}

template<class T> void reorder(QTreeWidget *list, std::vector<T> &vec)
{
	std::vector<T> out;
	for (int i = 0; i < list->topLevelItemCount(); i++) {
		int idx = list->topLevelItem(i)->data(0, Qt::UserRole).toInt();
		if (idx >= 0 && idx < (int)vec.size())
			out.push_back(vec[idx]);
	}
	if (out.size() == vec.size())
		vec = std::move(out);
}

} // namespace

SettingsDialog::SettingsDialog(Engine *engine, Controller *controller, QWidget *parent)
	: QDialog(parent),
	  engine_(engine),
	  controller_(controller),
	  rules_(engine->rules())
{
	setWindowTitle(text("FocusFollower"));
	resize(900, 480);

	wait_ = new QSpinBox(this);
	wait_->setRange(0, 60);
	wait_->setSuffix(text("Seconds"));
	wait_->setValue(controller_->waitSeconds());
	auto *top = new QHBoxLayout();
	top->addWidget(new QLabel(text("Wait"), this));
	top->addWidget(wait_);
	top->addStretch();

	// Scene pane
	auto *sceneBox = new QGroupBox(text("SceneSwitch"), this);
	scenes_ = makeList({text("App"), text("Scene"), text("Status")}, sceneBox);
	auto *addScene = button("Add", sceneBox);
	editScene_ = button("Edit", sceneBox);
	removeScene_ = button("Remove", sceneBox);
	auto *sceneButtons = new QHBoxLayout();
	sceneButtons->addWidget(addScene);
	sceneButtons->addWidget(editScene_);
	sceneButtons->addWidget(removeScene_);
	sceneButtons->addStretch();
	auto *sceneLayout = new QVBoxLayout(sceneBox);
	sceneLayout->addWidget(scenes_);
	sceneLayout->addLayout(sceneButtons);

	// Source pane
	auto *sourceBox = new QGroupBox(text("SourceShow"), this);
	group_ = new QComboBox(sourceBox);
	group_->setMinimumContentsLength(12);
	auto *addGroup = button("Add", sourceBox);
	renameGroup_ = button("Rename", sourceBox);
	removeGroup_ = button("Remove", sourceBox);
	mode_ = new QComboBox(sourceBox);
	mode_->addItem(text("Mode.Exclusive"), (int)GroupMode::Exclusive);
	mode_->addItem(text("Mode.ZOrder"), (int)GroupMode::ZOrder);
	auto *groupRow = new QHBoxLayout();
	groupRow->addWidget(new QLabel(text("Group"), sourceBox));
	groupRow->addWidget(group_, 1);
	groupRow->addWidget(addGroup);
	groupRow->addWidget(renameGroup_);
	groupRow->addWidget(removeGroup_);
	groupRow->addWidget(mode_);

	entries_ = makeList({text("App"), text("Match"), text("Title"), text("Source"), text("Status")}, sourceBox);
	addEntry_ = button("Add", sourceBox);
	editEntry_ = button("Edit", sourceBox);
	removeEntry_ = button("Remove", sourceBox);
	auto *entryButtons = new QHBoxLayout();
	entryButtons->addWidget(addEntry_);
	entryButtons->addWidget(editEntry_);
	entryButtons->addWidget(removeEntry_);
	entryButtons->addStretch();
	auto *sourceLayout = new QVBoxLayout(sourceBox);
	sourceLayout->addLayout(groupRow);
	sourceLayout->addWidget(entries_);
	sourceLayout->addLayout(entryButtons);

	auto *split = new QSplitter(Qt::Horizontal, this);
	split->addWidget(sceneBox);
	split->addWidget(sourceBox);
	split->setStretchFactor(0, 2);
	split->setStretchFactor(1, 3);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(top);
	layout->addWidget(split, 1);
	layout->addWidget(buttons);

	connect(addScene, &QPushButton::clicked, this, &SettingsDialog::addScene);
	connect(editScene_, &QPushButton::clicked, this, &SettingsDialog::editScene);
	connect(removeScene_, &QPushButton::clicked, this, &SettingsDialog::removeScene);
	connect(scenes_, &QTreeWidget::itemDoubleClicked, this, &SettingsDialog::editScene);
	connect(scenes_, &QTreeWidget::currentItemChanged, this, &SettingsDialog::updateButtons);
	connect(addGroup, &QPushButton::clicked, this, &SettingsDialog::addGroup);
	connect(renameGroup_, &QPushButton::clicked, this, &SettingsDialog::renameGroup);
	connect(removeGroup_, &QPushButton::clicked, this, &SettingsDialog::removeGroup);
	connect(group_, &QComboBox::currentIndexChanged, this, &SettingsDialog::groupChanged);
	connect(mode_, &QComboBox::currentIndexChanged, this, &SettingsDialog::modeChanged);
	connect(addEntry_, &QPushButton::clicked, this, &SettingsDialog::addEntry);
	connect(editEntry_, &QPushButton::clicked, this, &SettingsDialog::editEntry);
	connect(removeEntry_, &QPushButton::clicked, this, &SettingsDialog::removeEntry);
	connect(entries_, &QTreeWidget::itemDoubleClicked, this, &SettingsDialog::editEntry);
	connect(entries_, &QTreeWidget::currentItemChanged, this, &SettingsDialog::updateButtons);

	// Reordering by drag and drop arrives as row insertion.
	for (QTreeWidget *list : {scenes_, entries_}) {
		connect(list->model(), &QAbstractItemModel::rowsInserted, this, [this]() {
			if (updating_)
				return;
			QTimer::singleShot(0, this, [this]() {
				syncOrder();
				refresh();
			});
		});
	}

	refresh();
}

int SettingsDialog::currentGroup() const
{
	int g = group_->currentIndex();
	return g >= 0 && g < (int)rules_.groups.size() ? g : -1;
}

void SettingsDialog::syncOrder()
{
	reorder(scenes_, rules_.scenes);
	if (shownGroup_ >= 0 && shownGroup_ < (int)rules_.groups.size())
		reorder(entries_, rules_.groups[shownGroup_].entries);
}

std::vector<Target> SettingsDialog::sceneTargets() const
{
	std::vector<Target> out;
	obs_frontend_source_list list = {};
	obs_frontend_get_scenes(&list);
	for (size_t i = 0; i < list.sources.num; i++) {
		obs_source_t *s = list.sources.array[i];
		out.push_back({obs_source_get_uuid(s), obs_source_get_name(s)});
	}
	obs_frontend_source_list_free(&list);
	return out;
}

std::vector<Target> SettingsDialog::sourceTargets(int group, int editing) const
{
	std::set<std::string> otherGroups;
	for (int g = 0; g < (int)rules_.groups.size(); g++)
		if (g != group)
			for (const SourceEntry &e : rules_.groups[g].entries)
				otherGroups.insert(e.source);

	std::vector<OBSSource> members;
	const SourceGroup &grp = rules_.groups[group];
	if (grp.mode == GroupMode::ZOrder) {
		std::set<std::string> seen;
		for (int i = 0; i < (int)grp.entries.size(); i++) {
			if (i == editing || !seen.insert(grp.entries[i].source).second)
				continue;
			OBSSourceAutoRelease src = sourceByUuid(grp.entries[i].source);
			if (src)
				members.emplace_back(src.Get());
		}
	}

	std::vector<OBSSource> candidates;
	obs_enum_sources(
		[](void *param, obs_source_t *s) {
			if (obs_source_get_output_flags(s) & OBS_SOURCE_VIDEO)
				static_cast<std::vector<OBSSource> *>(param)->emplace_back(s);
			return true;
		},
		&candidates);
	for (const Target &t : sceneTargets()) {
		OBSSourceAutoRelease s = sourceByUuid(t.uuid);
		if (s)
			candidates.emplace_back(s.Get());
	}

	std::vector<Target> out;
	for (OBSSource &c : candidates) {
		std::string uuid = obs_source_get_uuid(c);
		if (otherGroups.count(uuid))
			continue;
		if (!members.empty()) {
			std::vector<obs_source_t *> set(members.begin(), members.end());
			set.push_back(c);
			if (!shareParent(set))
				continue;
		}
		out.push_back({uuid, obs_source_get_name(c)});
	}
	std::sort(out.begin(), out.end(), [](const Target &a, const Target &b) { return a.name < b.name; });
	return out;
}

bool SettingsDialog::zOrderPossible(int group) const
{
	std::vector<OBSSource> members;
	std::set<std::string> seen;
	for (const SourceEntry &e : rules_.groups[group].entries) {
		if (!seen.insert(e.source).second)
			continue;
		OBSSourceAutoRelease src = sourceByUuid(e.source);
		if (src)
			members.emplace_back(src.Get());
	}
	std::vector<obs_source_t *> raw(members.begin(), members.end());
	return shareParent(raw);
}

void SettingsDialog::refresh()
{
	updating_ = true;
	Report report = checkRules(rules_);

	int sceneRow = scenes_->currentIndex().row();
	scenes_->clear();
	for (int i = 0; i < (int)rules_.scenes.size(); i++) {
		SceneRule &s = rules_.scenes[i];
		scenes_->addTopLevelItem(makeItem(i, {qs(s.exe), qs(liveName(s.scene, s.sceneName)),
						      QString::fromUtf8(problemText(report.scenes[i]))}));
	}
	if (sceneRow >= 0 && sceneRow < scenes_->topLevelItemCount())
		scenes_->setCurrentItem(scenes_->topLevelItem(sceneRow));

	int g = group_->currentIndex();
	group_->clear();
	for (const SourceGroup &grp : rules_.groups)
		group_->addItem(qs(grp.name));
	if (g < 0 && !rules_.groups.empty())
		g = 0;
	g = std::min(g, (int)rules_.groups.size() - 1);
	group_->setCurrentIndex(g);

	int entryRow = entries_->currentIndex().row();
	entries_->clear();
	bool hasGroup = g >= 0;
	if (hasGroup) {
		SourceGroup &grp = rules_.groups[g];
		mode_->setCurrentIndex(mode_->findData((int)grp.mode));
		auto *model = qobject_cast<QStandardItemModel *>(mode_->model());
		if (model)
			model->item(1)->setEnabled(grp.mode == GroupMode::ZOrder || zOrderPossible(g));

		QString groupProblem = QString::fromUtf8(problemText(report.groups[g]));
		for (int i = 0; i < (int)grp.entries.size(); i++) {
			SourceEntry &e = grp.entries[i];
			QString status = QString::fromUtf8(problemText(report.entries[g][i]));
			if (status.isEmpty())
				status = groupProblem;
			if (status.isEmpty() && report.priorityWarnings[g][i])
				status = text("Warn.Priority");
			entries_->addTopLevelItem(
				makeItem(i, {qs(e.exe), titleModeText(e.title.mode), qs(e.title.pattern),
					     qs(liveName(e.source, e.sourceName)), status}));
		}
		if (entryRow >= 0 && entryRow < entries_->topLevelItemCount())
			entries_->setCurrentItem(entries_->topLevelItem(entryRow));
	}
	shownGroup_ = g;
	updating_ = false;
	updateButtons();
}

void SettingsDialog::updateButtons()
{
	bool hasGroup = currentGroup() >= 0;
	mode_->setEnabled(hasGroup);
	renameGroup_->setEnabled(hasGroup);
	removeGroup_->setEnabled(hasGroup);
	addEntry_->setEnabled(hasGroup);
	entries_->setEnabled(hasGroup);
	editEntry_->setEnabled(entries_->currentItem() != nullptr);
	removeEntry_->setEnabled(entries_->currentItem() != nullptr);
	editScene_->setEnabled(scenes_->currentItem() != nullptr);
	removeScene_->setEnabled(scenes_->currentItem() != nullptr);
}

void SettingsDialog::addScene()
{
	syncOrder();
	RuleDialog dlg(false, sceneTargets(), this);
	if (dlg.exec() != QDialog::Accepted)
		return;
	Target t = dlg.target();
	rules_.scenes.push_back({dlg.exe(), t.uuid, t.name});
	refresh();
	scenes_->setCurrentItem(scenes_->topLevelItem(scenes_->topLevelItemCount() - 1));
}

void SettingsDialog::editScene()
{
	syncOrder();
	int i = selectedIndex(scenes_);
	if (i < 0)
		return;
	SceneRule &s = rules_.scenes[i];
	RuleDialog dlg(false, sceneTargets(), this);
	dlg.setValues(s.exe, TitleMatch(), s.scene);
	if (dlg.exec() != QDialog::Accepted)
		return;
	Target t = dlg.target();
	s = {dlg.exe(), t.uuid, t.name};
	refresh();
}

void SettingsDialog::removeScene()
{
	syncOrder();
	int i = selectedIndex(scenes_);
	if (i < 0)
		return;
	rules_.scenes.erase(rules_.scenes.begin() + i);
	refresh();
}

void SettingsDialog::addGroup()
{
	syncOrder();
	SourceGroup g;
	g.name = text("NewGroup").arg(rules_.groups.size() + 1).toStdString();
	rules_.groups.push_back(g);
	updating_ = true;
	group_->addItem(qs(g.name));
	group_->setCurrentIndex(group_->count() - 1);
	updating_ = false;
	refresh();
}

void SettingsDialog::renameGroup()
{
	int g = currentGroup();
	if (g < 0)
		return;
	bool ok = false;
	QString name = QInputDialog::getText(this, text("Rename"), text("Group"), QLineEdit::Normal,
					     qs(rules_.groups[g].name), &ok);
	if (!ok || name.trimmed().isEmpty())
		return;
	syncOrder();
	rules_.groups[g].name = name.trimmed().toStdString();
	refresh();
}

void SettingsDialog::removeGroup()
{
	int g = currentGroup();
	if (g < 0)
		return;
	syncOrder();
	rules_.groups.erase(rules_.groups.begin() + g);
	shownGroup_ = -1;
	updating_ = true;
	group_->setCurrentIndex(std::min(g, (int)rules_.groups.size() - 1));
	updating_ = false;
	refresh();
}

void SettingsDialog::groupChanged(int)
{
	if (updating_)
		return;
	syncOrder();
	refresh();
}

void SettingsDialog::modeChanged(int)
{
	if (updating_)
		return;
	int g = currentGroup();
	if (g < 0)
		return;
	syncOrder();
	rules_.groups[g].mode = (GroupMode)mode_->currentData().toInt();
	refresh();
}

void SettingsDialog::addEntry()
{
	syncOrder();
	int g = currentGroup();
	if (g < 0)
		return;
	RuleDialog dlg(true, sourceTargets(g, -1), this);
	if (dlg.exec() != QDialog::Accepted)
		return;
	Target t = dlg.target();
	rules_.groups[g].entries.push_back({dlg.exe(), dlg.title(), t.uuid, t.name});
	refresh();
	entries_->setCurrentItem(entries_->topLevelItem(entries_->topLevelItemCount() - 1));
}

void SettingsDialog::editEntry()
{
	syncOrder();
	int g = currentGroup();
	int i = selectedIndex(entries_);
	if (g < 0 || i < 0)
		return;
	SourceEntry &e = rules_.groups[g].entries[i];
	RuleDialog dlg(true, sourceTargets(g, i), this);
	dlg.setValues(e.exe, e.title, e.source);
	if (dlg.exec() != QDialog::Accepted)
		return;
	Target t = dlg.target();
	e = {dlg.exe(), dlg.title(), t.uuid, t.name};
	refresh();
}

void SettingsDialog::removeEntry()
{
	syncOrder();
	int g = currentGroup();
	int i = selectedIndex(entries_);
	if (g < 0 || i < 0)
		return;
	rules_.groups[g].entries.erase(rules_.groups[g].entries.begin() + i);
	refresh();
}

void SettingsDialog::accept()
{
	syncOrder();
	engine_->setRules(rules_);
	controller_->setWaitSeconds(wait_->value());
	controller_->save();
	obs_frontend_save();
	QDialog::accept();
}
