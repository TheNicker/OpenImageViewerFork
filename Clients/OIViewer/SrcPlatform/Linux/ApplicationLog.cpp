#include "ApplicationLog.h"

#include <filesystem>
#include <fstream>

namespace OIV
{
    struct ApplicationLog::NativeState
    {
        LLUtils::native_string_type path;
    };

    ApplicationLog::ApplicationLog(LLUtils::native_string_type logPath, bool clear)
        : fNativeState(std::make_unique<NativeState>(NativeState{.path = std::move(logPath)}))
    {
        const std::filesystem::path path(fNativeState->path);
        std::filesystem::create_directories(path.parent_path());
        if (clear)
            std::ofstream(path, std::ios::trunc);
    }

    ApplicationLog::~ApplicationLog() = default;
    void ApplicationLog::Register() {}

    void ApplicationLog::Log(const LLUtils::native_string_type& message)
    {
        std::ofstream stream(std::filesystem::path(fNativeState->path), std::ios::app);
        stream << message;
    }

    const LLUtils::native_string_type& ApplicationLog::GetLogPath() const
    {
        return fNativeState->path;
    }
}  // namespace OIV
