#include "pch.h"
#include "GPUCollectors.h"
#include "Common/Helpers.h"

GPUDisjointTimer::GPUDisjointTimer(ID3D11Device* device, ID3D11DeviceContext* m_pDeviceContext, const std::string& name)
	: ProfilerCollector(name), m_pDeviceContext(m_pDeviceContext), m_Frequency(0)
{
	D3D11_QUERY_DESC queryDesc = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };

	APP_LOG_IF_ERROR(FAILED(device->CreateQuery(&queryDesc, &m_pDisjointTimerQuery)), L"Could not create start Timestamp query");
}

void GPUDisjointTimer::PrintQueryData()
{
	APP_LOG_INFO(StringHelpers::StringFormat(L"GPU Frequency = 1/%d, %.3f", Profiler::GetInstance().m_GPUFrequency, Profiler::GetInstance().m_GPUInvFrequency));
}

void GPUDisjointTimer::ProcessQuery()
{
	D3D10_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
	while (FAILED(m_pDeviceContext->GetData(m_pDisjointTimerQuery, &tsDisjoint, sizeof(tsDisjoint), 0)));

	if (!tsDisjoint.Disjoint)
	{
		Profiler::GetInstance().m_GPUFrequency = tsDisjoint.Frequency;
		Profiler::GetInstance().m_GPUInvFrequency = 1.f / float(tsDisjoint.Frequency);
	}
}

bool GPUDisjointTimer::Start_Imp()
{
	m_pDeviceContext->Begin(m_pDisjointTimerQuery);
	return true;
}

bool GPUDisjointTimer::Stop_Imp()
{
	m_pDeviceContext->End(m_pDisjointTimerQuery);
	return true;
}




GPUTimer::GPUTimer(ID3D11Device* device, ID3D11DeviceContext* m_pDeviceContext, const std::string& name)
	: ProfilerCollector(name), m_pDeviceContext(m_pDeviceContext), m_DurationMS(0)
{
	D3D11_QUERY_DESC queryDesc = { D3D11_QUERY_TIMESTAMP, 0 };

	APP_LOG_IF_ERROR(FAILED(device->CreateQuery(&queryDesc, &m_pStartTimerQuery)), L"Could not create start Timestamp query");
	APP_LOG_IF_ERROR(FAILED(device->CreateQuery(&queryDesc, &m_pEndTimerQuery)), L"Could not create end Timestamp query");
}

void GPUTimer::PrintQueryData()
{
	APP_LOG_INFO(StringHelpers::StringFormat(L"%s: Duration : %.4f", GetName(), m_DurationMS));
}

void GPUTimer::ProcessQuery()
{
	uint64_t timeStampStart, timeStampEnd;
	while (FAILED(m_pDeviceContext->GetData(m_pStartTimerQuery, &timeStampStart, sizeof(uint64_t), 0)));
	while (FAILED(m_pDeviceContext->GetData(m_pEndTimerQuery, &timeStampEnd, sizeof(uint64_t), 0)));

	m_DurationMS = float(timeStampEnd - timeStampStart) * Profiler::GetInstance().m_GPUInvFrequency * 1000.f;
}

bool GPUTimer::Start_Imp()
{
	m_pDeviceContext->End(m_pStartTimerQuery);
	return true;
}

bool GPUTimer::Stop_Imp()
{
	m_pDeviceContext->End(m_pEndTimerQuery);
	return true;
}