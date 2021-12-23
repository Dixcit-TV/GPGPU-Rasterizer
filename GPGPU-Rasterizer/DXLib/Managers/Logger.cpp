#include "pch.h"
#include "Logger.h"

Logger::Logger()
	: m_ConsoleHandle{ GetStdHandle(STD_OUTPUT_HANDLE) }
{}

void Logger::Log(ELogSeverity severity, const std::wstring& message, bool assertion, UINT lineNumber, const std::wstring& file)
{
	std::wstringstream ss;
	switch(severity)
	{
	case ELogSeverity::Info:
		SetConsoleTextAttribute(m_ConsoleHandle, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		ss << L"--INFO: ";
		break;
	case ELogSeverity::Warning:
		SetConsoleTextAttribute(m_ConsoleHandle, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
		ss << L"--WARNING: ";
		break;
	case ELogSeverity::Error:
		SetConsoleTextAttribute(m_ConsoleHandle, FOREGROUND_INTENSITY | FOREGROUND_RED);
		ss << L"--ERROR: ";
		break;
	default: return;
	}

	if (!assertion)
	{
		ss << file << " at line " << std::to_wstring(lineNumber) << ": " << message << std::endl;
		std::wcout << ss.str();

		if (severity == ELogSeverity::Error) __debugbreak();
	}

	SetConsoleTextAttribute(m_ConsoleHandle, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}
