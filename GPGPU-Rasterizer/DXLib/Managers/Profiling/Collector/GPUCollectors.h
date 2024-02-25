#pragma once
#include "Managers/Logger.h"
#include "Managers/Profiling/Profiler.h"
#include <string>

class GPUDisjointTimer final : public ProfilerCollector
{
public:
	explicit GPUDisjointTimer(ID3D11Device* device, ID3D11DeviceContext* m_pDeviceContext, const std::string& name);
	void PrintQueryData() override;
	void ProcessQuery() override;

private:
	bool Start_Imp() override;
	bool Stop_Imp() override;

private:
	ID3D11Query* m_pDisjointTimerQuery;
	ID3D11DeviceContext* m_pDeviceContext;

	uint64_t m_Frequency;
};

class GPUTimer final : public ProfilerCollector
{
public:
	explicit GPUTimer(ID3D11Device* device, ID3D11DeviceContext* m_pDeviceContext, const std::string& name);
	void PrintQueryData() override;
	void ProcessQuery() override;

private:
	bool Start_Imp() override;
	bool Stop_Imp() override;

private:
	ID3D11Query* m_pStartTimerQuery;
	ID3D11Query* m_pEndTimerQuery;
	ID3D11DeviceContext* m_pDeviceContext;

	float m_DurationMS;
}