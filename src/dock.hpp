#pragma once

#include <QWidget>

class Controller;
class Engine;
class QCheckBox;

class Dock : public QWidget {
	Q_OBJECT

public:
	Dock(Controller *controller, Engine *engine, QWidget *parent = nullptr);

private slots:
	void openSettings();

private:
	Controller *controller_;
	Engine *engine_;
	QCheckBox *enabled_;
};
