#include "dock.hpp"
#include "controller.hpp"
#include "engine.hpp"
#include "ui/settings-dialog.hpp"

#include <obs-module.h>

#include <QCheckBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

Dock::Dock(Controller *controller, Engine *engine, QWidget *parent)
	: QWidget(parent),
	  controller_(controller),
	  engine_(engine)
{
	enabled_ = new QCheckBox(obs_module_text("Enabled"), this);
	enabled_->setChecked(controller->enabled());
	auto *settings = new QPushButton(obs_module_text("Settings"), this);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(enabled_);
	layout->addWidget(settings);
	layout->addStretch();

	connect(enabled_, &QCheckBox::toggled, controller, &Controller::setEnabled);
	connect(controller, &Controller::enabledChanged, this, [this](bool on) {
		QSignalBlocker block(enabled_);
		enabled_->setChecked(on);
	});
	connect(settings, &QPushButton::clicked, this, &Dock::openSettings);
}

void Dock::openSettings()
{
	SettingsDialog dlg(engine_, controller_, window());
	dlg.exec();
}
