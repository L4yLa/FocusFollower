#include "rule-dialog.hpp"
#include "platform/window-system.hpp"

#include <obs-module.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>

using namespace ff;

namespace {

QString qs(const std::string &s)
{
	return QString::fromStdString(s);
}

} // namespace

RuleDialog::RuleDialog(bool withTitle, const std::vector<Target> &targets, QWidget *parent)
	: QDialog(parent),
	  withTitle_(withTitle)
{
	setWindowTitle(obs_module_text("Rule"));

	app_ = new QComboBox(this);
	for (const RunningApp &a : runningApps()) {
		apps_.emplace_back(a.exe, a.titles);
		app_->addItem(qs(a.exe));
	}

	auto *form = new QFormLayout(this);
	form->addRow(obs_module_text("App"), app_);

	if (withTitle_) {
		useTitle_ = new QCheckBox(this);
		titleMode_ = new QComboBox(this);
		titleMode_->addItem(obs_module_text("TitleMode.Exact"), (int)TitleMode::Exact);
		titleMode_->addItem(obs_module_text("TitleMode.Partial"), (int)TitleMode::Partial);
		titleMode_->addItem(obs_module_text("TitleMode.Regex"), (int)TitleMode::Regex);
		pattern_ = new QComboBox(this);
		pattern_->setEditable(true);
		pattern_->setMinimumContentsLength(30);

		auto *row = new QHBoxLayout();
		row->addWidget(useTitle_);
		row->addWidget(titleMode_);
		row->addWidget(pattern_, 1);
		form->addRow(obs_module_text("Title"), row);

		connect(useTitle_, &QCheckBox::toggled, this, &RuleDialog::updateState);
		connect(titleMode_, &QComboBox::currentIndexChanged, this, &RuleDialog::updateState);
		connect(pattern_, &QComboBox::editTextChanged, this, &RuleDialog::updateState);
		connect(app_, &QComboBox::currentIndexChanged, this, &RuleDialog::updateTitles);
	}

	target_ = new QComboBox(this);
	for (const Target &t : targets)
		target_->addItem(qs(t.name), qs(t.uuid));
	form->addRow(obs_module_text(withTitle_ ? "Source" : "Scene"), target_);

	buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	form->addRow(buttons_);
	connect(buttons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(app_, &QComboBox::currentIndexChanged, this, &RuleDialog::updateState);
	connect(target_, &QComboBox::currentIndexChanged, this, &RuleDialog::updateState);

	updateTitles();
	updateState();
}

void RuleDialog::selectExe(const std::string &exe)
{
	for (int i = 0; i < app_->count(); i++) {
		if (exeEquals(app_->itemText(i).toStdString(), exe)) {
			app_->setCurrentIndex(i);
			return;
		}
	}
	apps_.emplace_back(exe, std::vector<std::string>());
	app_->addItem(qs(exe));
	app_->setCurrentIndex(app_->count() - 1);
}

void RuleDialog::setValues(const std::string &exe, const TitleMatch &title, const std::string &target)
{
	selectExe(exe);
	if (withTitle_) {
		bool use = title.mode != TitleMode::None;
		useTitle_->setChecked(use);
		if (use)
			titleMode_->setCurrentIndex(titleMode_->findData((int)title.mode));
		pattern_->setEditText(qs(title.pattern));
	}
	int i = target_->findData(qs(target));
	if (i >= 0)
		target_->setCurrentIndex(i);
	updateState();
}

void RuleDialog::updateTitles()
{
	if (!withTitle_)
		return;
	QString text = pattern_->currentText();
	pattern_->clear();
	int i = app_->currentIndex();
	if (i >= 0 && i < (int)apps_.size())
		for (const std::string &t : apps_[i].second)
			pattern_->addItem(qs(t));
	pattern_->setEditText(text);
}

void RuleDialog::updateState()
{
	bool ok = app_->currentIndex() >= 0 && target_->currentIndex() >= 0;
	if (withTitle_) {
		bool use = useTitle_->isChecked();
		titleMode_->setEnabled(use);
		pattern_->setEnabled(use);
		if (use) {
			TitleMatch t = title();
			ok = ok && !t.pattern.empty() && t.valid();
		}
	}
	buttons_->button(QDialogButtonBox::Ok)->setEnabled(ok);
}

std::string RuleDialog::exe() const
{
	return app_->currentText().toStdString();
}

TitleMatch RuleDialog::title() const
{
	TitleMatch t;
	if (withTitle_ && useTitle_->isChecked()) {
		t.mode = (TitleMode)titleMode_->currentData().toInt();
		t.pattern = pattern_->currentText().toStdString();
	}
	return t;
}

Target RuleDialog::target() const
{
	return Target{target_->currentData().toString().toStdString(), target_->currentText().toStdString()};
}
