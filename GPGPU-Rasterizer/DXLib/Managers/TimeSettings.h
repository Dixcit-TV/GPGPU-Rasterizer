#pragma once
#include <chrono>
#include "Singleton.h"

class TimeSettings final : public Singleton<TimeSettings>
{
public:

	~TimeSettings() override = default;
	TimeSettings(const TimeSettings & other) = delete;
	TimeSettings(TimeSettings && other) = delete;
	TimeSettings& operator=(const TimeSettings & other) = delete;
	TimeSettings& operator=(TimeSettings && other) = delete;

	void Update();
	void TrySleep() const;
	//bool CatchUp();

	float GetElapsed() const { return m_ElapsedTime; }
	unsigned int GetFPS() const { return m_FPS; }

private:
	friend class Singleton<TimeSettings>;
	explicit TimeSettings();

	using TimePoint = std::chrono::high_resolution_clock::time_point;
	using Clock = std::chrono::high_resolution_clock;

	TimePoint m_LastTimePoint;
	
	float m_ElapsedTime;
	float m_Lag;
	float m_FPSTimerCount;
	unsigned int m_FPS;
	unsigned int m_FPSCounter;

	const float FIXED_DELTA_TIME = 0.2f;
	const float MAX_FRAME_TIME = 30.f;
	const int FRAME_TIME_MS = 16;
	const bool USE_FIXED_FRAME_TIME = false;
};

