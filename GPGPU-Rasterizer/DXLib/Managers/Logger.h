#pragma once
#include "Singleton.h"

enum class ELogSeverity
{
	Info, Warning, Error
};

#define WFILE1(x) L##x
#define WFILE(x) WFILE1(x)

#if defined(DEBUG) | defined(_DEBUG)
	#define APP_LOG(severity, message, assertion, appendLineAndFile) Logger::GetInstance().Log(severity, message, assertion, appendLineAndFile, __LINE__, WFILE(__FILE__));
#else
	#define APP_LOG(severity, message, assertion)
#endif

#define APP_LOG_INFO(message) APP_LOG(ELogSeverity::Info, message, false, false)
#define APP_LOG_WARNING(message, assertion) APP_LOG(ELogSeverity::Warning, message, assertion, true)
#define APP_LOG_ERROR(message, assertion) APP_LOG(ELogSeverity::Error, message, assertion, true)

class Logger : public Singleton<Logger>
{
public:
	~Logger() override = default;
	Logger(const Logger& other) = delete;
	Logger(Logger&& other) = delete;
	Logger& operator=(const Logger& other) = delete;
	Logger& operator=(Logger&& other) = delete;

	/**
	 * \brief : Log messages to the console, use APP_LOG, APP_LOG_INFO, APP_LOG_WARNING, APP_LOG_ERROR macros instead as they append file and line number automatically.\n
	 * Logging is automatically disabled in Release
	 * \param severity : Severity of the message
	 * \param message : message printed to the console
	 * \param assertion : assertion check to trigger logging message, message is logged if the assertion is false.
	 * \param appendLineAndFile : Defines whether the line number and fgile name should be appended to the message
	 * \param lineNumber : Line the Logging was called from
	 * \param file : File the Logging was called from
	 */
	void Log(ELogSeverity severity, const std::wstring& message, bool assertion, bool appendLineAndFile, UINT lineNumber, const std::wstring& file) const;

private:
	friend class Singleton<Logger>;
	explicit Logger();

	HANDLE m_ConsoleHandle;
};

