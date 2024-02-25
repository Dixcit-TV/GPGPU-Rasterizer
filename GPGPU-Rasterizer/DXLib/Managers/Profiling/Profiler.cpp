#include "pch.h"
#include "Profiler.h"
#include <winerror.h>

void ProfilerCollector::Start()
{
	m_HasStarted = Start_Imp();
}

void ProfilerCollector::Stop()
{
	m_HasStoped = Stop_Imp();
}