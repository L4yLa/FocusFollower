#pragma once

#include "core/rules.hpp"

#include <QDialog>

#include <string>
#include <utility>
#include <vector>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;

struct Target {
	std::string uuid;
	std::string name;
};

class RuleDialog : public QDialog {
	Q_OBJECT

public:
	// withTitle selects the source form; the scene form has no title condition.
	RuleDialog(bool withTitle, const std::vector<Target> &targets, QWidget *parent = nullptr);

	void setValues(const std::string &exe, const ff::TitleMatch &title, const std::string &target);
	std::string exe() const;
	ff::TitleMatch title() const;
	Target target() const;

private slots:
	void updateTitles();
	void updateState();

private:
	void selectExe(const std::string &exe);

	bool withTitle_;
	QComboBox *app_;
	QCheckBox *useTitle_ = nullptr;
	QComboBox *titleMode_ = nullptr;
	QComboBox *pattern_ = nullptr;
	QComboBox *target_;
	QDialogButtonBox *buttons_;
	std::vector<std::pair<std::string, std::vector<std::string>>> apps_;
};
