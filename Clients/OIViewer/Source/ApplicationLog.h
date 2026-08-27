#pragma once

#include <LLUtils/StringDefs.h>

#include <memory>

namespace OIV
{
    class ApplicationLog final
    {
      public:

        ApplicationLog(LLUtils::native_string_type logPath, bool clear);
        ~ApplicationLog();

        ApplicationLog(const ApplicationLog&)            = delete;
        ApplicationLog& operator=(const ApplicationLog&) = delete;

        void Register();
        void Log(const LLUtils::native_string_type& message);
        [[nodiscard]] const LLUtils::native_string_type& GetLogPath() const;

      private:

        struct NativeState;
        std::unique_ptr<NativeState> fNativeState;
    };
}  // namespace OIV
