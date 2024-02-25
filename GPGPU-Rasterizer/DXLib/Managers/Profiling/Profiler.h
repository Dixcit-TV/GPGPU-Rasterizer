#pragma once
#include "Managers/Singleton.h"
#include "Managers/Logger.h"
#include <string>

class ProfilerCollector
{
public:
	virtual void PrintQueryData() = 0;
	virtual void ProcessQuery() = 0;

	void Start();
	void Stop();

	const std::string& GetName() const { return m_Name; }
	bool HasStarted() const { return m_HasStarted; }
	bool HasStoped() const { return m_HasStarted; }

	const std::string& GetName() const { return m_Name; }

protected:
	ProfilerCollector(const std::string& name)
		: m_Name(name), m_HasStarted(false), m_HasStoped(false) {}

	virtual bool Start_Imp() { return false; }
	virtual bool Stop_Imp() { return false; }

private:
	const std::string m_Name;
	bool m_HasStarted;
	bool m_HasStoped;
};

struct ProfilingHandle
{
public:
	explicit ProfilingHandle(ProfilerCollector* m_pCollector, bool m_IsScoped = false)
		: m_pCollector(m_pCollector)
		, m_IsScoped(m_IsScoped)
	{
		if (m_IsScoped && !m_pCollector->HasStarted())
		{
			m_pCollector->Start();
			APP_ASSERT_WARNING(!m_pCollector->HasStarted(), L"Scoped ProfilerCollector was not stopped when the created.");
		}
	}

	~ProfilingHandle()
	{
		if (m_IsScoped && !m_pCollector->HasStoped())
			m_pCollector->Stop();

		APP_ASSERT_WARNING(!m_pCollector->HasStoped(), L"ProfilerCollector was not stopped when the destroyed, set the Profiling as scopped or add an explicit call to Stop.");
	}

private:
	ProfilerCollector* m_pCollector = nullptr;
	bool m_IsScoped = false;
};
	
class Profiler : public Singleton<Profiler>
{
public:
	~Profiler() override = default;
	Profiler(const Profiler& other) = delete;
	Profiler(Profiler&& other) = delete;
	Profiler& operator=(const Profiler& other) = delete;
	Profiler& operator=(Profiler&& other) = delete;

private:
	friend class Singleton<Profiler>;
	explicit Profiler();

public:
	uint64_t m_GPUFrequency = 0;
	float m_GPUInvFrequency = 0.f;
};

