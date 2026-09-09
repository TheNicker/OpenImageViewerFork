#include "Main.h"

#include "ViewerApplication.h"

#include <LLUtils/Exception.h>
#include <LWS/Platform.hpp>
#ifdef LWS_HAS_WIN32_BACKEND
    #include <LWS/Win32/Platform.hpp>
#endif

#include <cstdlib>
#include <stdexcept>

LLUtils::native_string_type CompileFilePathFromArguments(int argc, const LLUtils::native_char_type* const* argv)
{
    LLUtils::native_string_type filePath;
    if (argc > 1)
    {
        filePath = argv[1];
        for (int index = 2; index < argc; ++index)
            filePath += LLUtils::native_string_type(LLUTILS_TEXT(" ")) + argv[index];
    }
    return filePath;
}

int RunViewer(const LLUtils::native_string_type& filePath)
{
    try
    {
#ifdef LWS_HAS_WIN32_BACKEND
        if (LWS::Win32::BootstrapProcess() != LWS::Result::Success)
            throw std::runtime_error("Unable to configure Win32 process state");
#endif

        LWS::PlatformContext platform;
        const LWS::PlatformConfig platformConfig{
#ifdef LWS_HAS_WIN32_BACKEND
            .backend = LWS::BackendId::Win32,
#elif defined(LWS_HAS_WAYLAND_BACKEND)
            .backend = LWS::BackendId::Wayland,
#endif
        };
        if (platform.Init(platformConfig) != LWS::Result::Success)
            throw std::runtime_error("Unable to initialize the LWS platform");

        platform.SetUnhandledExceptionHandler(
            [](std::exception_ptr exception) noexcept
            {
                try
                {
                    std::rethrow_exception(exception);
                }
                catch (const std::exception& error)
                {
                    LL_EXCEPTION_DONT_THROW(LLUtils::Exception::ErrorCode::RuntimeError, error.what());
                }
                catch (...)
                {
                    LL_EXCEPTION_DONT_THROW(LLUtils::Exception::ErrorCode::Unknown, "Unhandled UI callback exception");
                }
            });
        {
            OIV::ViewerApplication viewerApplication(platform);
            viewerApplication.Init(filePath);
            viewerApplication.Run();
        }
        if (platform.Shutdown() != LWS::Result::Success)
            throw std::runtime_error("Unable to shut down the LWS platform");
        return EXIT_SUCCESS;
    }
    catch (const LLUtils::Exception&)
    {
        return EXIT_FAILURE;
    }
    catch (const std::exception& exception)
    {
        LL_EXCEPTION_DONT_THROW(LLUtils::Exception::ErrorCode::RuntimeError, exception.what());
        return EXIT_FAILURE;
    }
    catch (...)
    {
        LL_EXCEPTION_DONT_THROW(LLUtils::Exception::ErrorCode::Unknown, "Unhandled application exception");
        return EXIT_FAILURE;
    }
}
