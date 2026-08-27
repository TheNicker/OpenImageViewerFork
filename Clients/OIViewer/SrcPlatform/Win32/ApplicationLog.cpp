#include "ApplicationLog.h"

#include <LLUtils/Logging/Logger.h>
#include <LLUtils/Logging/LogPredefined.h>

namespace OIV
{
    struct ApplicationLog::NativeState
    {
        NativeState(LLUtils::native_string_type path, bool clear) : log(std::move(path), clear) {}

        LLUtils::LogFile log;
    };

    ApplicationLog::ApplicationLog(LLUtils::native_string_type logPath, bool clear)
        : fNativeState(std::make_unique<NativeState>(std::move(logPath), clear))
    {
    }

    ApplicationLog::~ApplicationLog() = default;

    void ApplicationLog::Register()
    {
        LLUtils::Logger::GetSingleton().AddLogTarget(&fNativeState->log);
    }

    void ApplicationLog::Log(const LLUtils::native_string_type& message)
    {
        fNativeState->log.Log(message);
    }

    const LLUtils::native_string_type& ApplicationLog::GetLogPath() const
    {
        return fNativeState->log.GetLogPath();
    }
}  // namespace OIV
