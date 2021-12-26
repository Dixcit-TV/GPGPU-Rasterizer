#include "pch.h"
#include "Logger.h"

Logger::Logger()
	: m_ConsoleHandle{ GetStdHandle(STD_OUTPUT_HANDLE) }
{}

void Logger::Log(ELogSeverity severity, const std::wstring& message, bool assertion, bool appendLineAndFile, UINT lineNumber, const std::wstring& file) const
{
	if (!assertion)
	{

		std::wstringstream ss{};
		switch(severity)
		{
		case ELogSeverity::Info:
			SetConsoleTextAttribute(m_ConsoleHandle, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE);
			ss << L"\t--INFO: ";
			break;
		case ELogSeverity::Warning:
			SetConsoleTextAttribute(m_ConsoleHandle, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
			ss << L"\t--WARNING: ";
			break;
		case ELogSeverity::Error:
			SetConsoleTextAttribute(m_ConsoleHandle, FOREGROUND_INTENSITY | FOREGROUND_RED);
			ss << L"\t--ERROR: ";
			break;
		default: return;
		}

		if (appendLineAndFile)
			ss << file << " at line " << std::to_wstring(lineNumber) << ": ";

		ss << message << std::endl;
		std::wcout << ss.str();

		SetConsoleTextAttribute(m_ConsoleHandle, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

		if (severity == ELogSeverity::Error) __debugbreak();
	}
}
