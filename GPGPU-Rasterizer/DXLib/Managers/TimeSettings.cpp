#include "pch.h"
#include "TimeSettings.h"
#include <algorithm>
#include <thread>

TimeSettings::TimeSettings()
	: m_LastTimePoint(Clock::now())
	, m_ElapsedTime(0.f)
	, m_Lag(0.f)
	, m_FPSTimerCount(0.f)
	, m_FPS(0)
	, m_FPSCounter(0)
{}

void TimeSettings::Update()
{
	const TimePoint currentTime{ Clock::now() };
	const float frameInterval{ 1.f };
	m_ElapsedTime = std::chrono::duration<float>(currentTime - m_LastTimePoint).count();
	m_LastTimePoint = currentTime;
	m_ElapsedTime = std::clamp(m_ElapsedTime, 0.f, MAXFRAMETIME);

	m_Lag += m_ElapsedTime;

	m_FPSTimerCount += m_ElapsedTime;
	++m_FPSCounter;

	if (m_FPSTimerCount >= frameInterval)
	{
		m_FPSTimerCount -= frameInterval;
		m_FPS = m_FPSCounter;
		m_FPSCounter = 0;
	}
}

//bool ngenius::TimeSettings::CatchUp()
//{
//	const bool shouldCatchup{ m_Lag >= FIXEDDELTATIME };
//
//	if (shouldCatchup)
//	{
//		m_Lag -= FIXEDDELTATIME;
//	}
//	
//	return shouldCatchup;
//}

void TimeSettings::TrySleep() const
{
	if (USEFIXEDFRAMETIME)
	{
		const auto sleepTime{ m_LastTimePoint + std::chrono::milliseconds(FRAMETIMEMS) - Clock::now() };
		std::this_thread::sleep_for(sleepTime);
	}
}