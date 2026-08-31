#include "ExceptionHandler.h"
#include "Main.h"
#include "CopyDataProtocol.h"
#include "ViewerApplication.h"

#include <LLUtils/Exception.h>
#include <LLUtils/FileSystemHelper.h>

#include <Windows.h>
#include <shellapi.h>

#include <cstdlib>

namespace
{
    class ExceptionRegistration final
    {
      public:

        ExceptionRegistration() { OIV::RegisterExceptionhandler(); }
        ~ExceptionRegistration() { OIV::RemoveExceptionHandler(); }
    };

    struct ExistingInstanceForwardingState
    {
        LWS::Handle targetWindow = 0;
    };

    void ForwardFileToExistingInstance(LWS::Handle targetWindow, const LLUtils::native_string_type& filePath)
    {
        COPYDATASTRUCT copyData{};
        copyData.dwData = OIV::Win32::LoadFileCopyDataId;
        copyData.cbData = static_cast<DWORD>((filePath.length() + 1) * sizeof(LLUtils::native_char_type));
        copyData.lpData = const_cast<LLUtils::native_char_type*>(filePath.c_str());
        SendMessage(reinterpret_cast<HWND>(targetWindow), WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&copyData));
    }

    int PlatformMain(int argc, const wchar_t* const* argv)
    {
        const ExceptionRegistration exceptionRegistration;
        try
        {
            LLUtils::native_string_type filePath = CompileFilePathFromArguments(argc, argv);
            ExistingInstanceForwardingState forwarding{.targetWindow = OIV::ViewerApplication::FindTrayBarWindow()};
            if (!filePath.empty() && forwarding.targetWindow != 0)
            {
                filePath = LLUtils::FileSystemHelper::ResolveFullPath(filePath);
                ForwardFileToExistingInstance(forwarding.targetWindow, filePath);
                return EXIT_SUCCESS;
            }
            return RunViewer(filePath);
        }
        catch (const LLUtils::Exception&)
        {
            return EXIT_FAILURE;
        }
        catch (...)
        {
            LL_EXCEPTION_DONT_THROW(LLUtils::Exception::ErrorCode::Unknown, "Unhandled entry-point exception");
            return EXIT_FAILURE;
        }
    }
}  // namespace

int WINAPI wWinMain([[maybe_unused]] HINSTANCE instance, [[maybe_unused]] HINSTANCE previousInstance,
                    [[maybe_unused]] PWSTR commandLine, [[maybe_unused]] int showCommand)
{
    int argumentCount   = 0;
    wchar_t** arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
    if (arguments == nullptr)
        return EXIT_FAILURE;

    const int result = PlatformMain(argumentCount, arguments);
    LocalFree(arguments);
    return result;
}
