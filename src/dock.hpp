#pragma once

#include <QWidget>

class Controller;
class QCheckBox;

class Dock : public QWidget {
	Q_OBJECT

public:
	explicit Dock(Controller *controller, QWidget *parent = nullptr);

private:
	QCheckBox *enabled_;
};
