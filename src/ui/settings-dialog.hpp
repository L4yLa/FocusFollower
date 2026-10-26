#pragma once

#include "core/rules.hpp"
#include "ui/rule-dialog.hpp"

#include <QDialog>

class Controller;
class Engine;
class QComboBox;
class QPushButton;
class QSpinBox;
class QTreeWidget;

class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	SettingsDialog(Engine *engine, Controller *controller, QWidget *parent = nullptr);

	void accept() override;

private slots:
	void addScene();
	void editScene();
	void removeScene();
	void addGroup();
	void renameGroup();
	void removeGroup();
	void groupChanged(int index);
	void modeChanged(int index);
	void addEntry();
	void editEntry();
	void removeEntry();
	void refresh();
	void updateButtons();

private:
	void syncOrder();
	int currentGroup() const;
	std::vector<Target> sceneTargets() const;
	std::vector<Target> sourceTargets(int group, int editing) const;
	bool zOrderPossible(int group) const;

	Engine *engine_;
	Controller *controller_;
	ff::Rules rules_;

	QSpinBox *wait_;
	QTreeWidget *scenes_;
	QComboBox *group_;
	QComboBox *mode_;
	QTreeWidget *entries_;
	QPushButton *renameGroup_;
	QPushButton *removeGroup_;
	QPushButton *addEntry_;
	QPushButton *editEntry_;
	QPushButton *removeEntry_;
	QPushButton *editScene_;
	QPushButton *removeScene_;
	int shownGroup_ = -1;
	bool updating_ = false;
};
