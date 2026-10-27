#pragma once

#include <obs.h>

#include <QObject>

#include <atomic>

class Controller : public QObject {
	Q_OBJECT

public:
	Controller();
	~Controller() override;

	bool enabled() const { return enabled_.load(); }
	int waitSeconds() const { return wait_; }
	void setWaitSeconds(int s);
	void load();
	void save() const;

public slots:
	void setEnabled(bool on);
	void toggle();

signals:
	void enabledChanged(bool on);

private:
	static void onHotkey(void *data, obs_hotkey_id id, obs_hotkey_t *key, bool pressed);

	std::atomic<bool> enabled_{true};
	int wait_ = 2;
	obs_hotkey_id hotkey_ = OBS_INVALID_HOTKEY_ID;
};
