#include "dock.hpp"
#include "controller.hpp"

#include <obs-module.h>

#include <QCheckBox>
#include <QSignalBlocker>
#include <QVBoxLayout>

Dock::Dock(Controller *controller, QWidget *parent) : QWidget(parent)
{
	enabled_ = new QCheckBox(obs_module_text("Enabled"), this);
	enabled_->setChecked(controller->enabled());

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(enabled_);
	layout->addStretch();

	connect(enabled_, &QCheckBox::toggled, controller, &Controller::setEnabled);
	connect(controller, &Controller::enabledChanged, this, [this](bool on) {
		QSignalBlocker block(enabled_);
		enabled_->setChecked(on);
	});
}
