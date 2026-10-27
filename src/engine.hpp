#pragma once

#include "core/logic.hpp"
#include "platform/window-system.hpp"

#include <obs.hpp>

#include <QObject>
#include <QTimer>

#include <memory>

class Controller;

class Engine : public QObject {
	Q_OBJECT

public:
	explicit Engine(Controller *controller);
	~Engine() override;

	const ff::Rules &rules() const { return rules_; }
	void setRules(const ff::Rules &rules);
	void clearState();
	void reset();
	void stop();

private slots:
	void tick();
	void onEnabledChanged(bool on);

private:
	void schedule();
	void logStats();
	void apply(const ff::WindowInfo &w);
	void applyGroup(obs_source_t *front, const ff::SourceGroup &group, const ff::SourceEntry &entry, uint64_t hwnd);

	Controller *controller_;
	ff::Rules rules_;
	ff::Dwell dwell_;
	ff::Watches watches_;
	QTimer poll_;
	QTimer statsTimer_;
	uint64_t ticks_ = 0;
	uint64_t tickNs_ = 0;
	uint64_t tickMaxNs_ = 0;
	bool scheduled_ = false;
	std::unique_ptr<ff::WindowEvents> events_;
};
