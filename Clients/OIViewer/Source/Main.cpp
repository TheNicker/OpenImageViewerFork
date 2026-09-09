#include "Main.h"

#include "ViewerApplication.h"

#include "OIV.h"

#include <LLUtils/Exception.h>
#include <LLUtils/StringUtility.h>
#include <LWS/Platform.hpp>
#ifdef LWS_HAS_WIN32_BACKEND
    #include <LWS/Win32/Platform.hpp>
#endif

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <string>
#include <stdexcept>

namespace
{
    int gPreferredGPUIndex = -1;

    LLUtils::native_char_type ToLowerAscii(LLUtils::native_char_type character)
    {
        const auto upperA = static_cast<LLUtils::native_char_type>('A');
        const auto upperZ = static_cast<LLUtils::native_char_type>('Z');
        const auto lowerA = static_cast<LLUtils::native_char_type>('a');
        return character >= upperA && character <= upperZ ? character + (lowerA - upperA) : character;
    }

    int ParseGpuIndex(const LLUtils::native_string_type& value)
    {
        const std::string narrowValue = LLUtils::StringUtility::ConvertString<std::string>(value);
        int gpuIndex{};
        const auto result = std::from_chars(narrowValue.data(), narrowValue.data() + narrowValue.size(), gpuIndex);
        if (result.ec != std::errc{} || result.ptr != narrowValue.data() + narrowValue.size() || gpuIndex < 0)
            throw std::invalid_argument("--gpu requires a non-negative integer index");
        return gpuIndex;
    }

    void PrintHelp()
    {
        std::cout << "\033[1;36m" << "OpenImageViewer - Image Viewer" << "\033[0m" << "\n\n";

        std::cout << "\033[1;33m" << "USAGE" << "\033[0m" << "\n";
        std::cout << "  OIViewer [options] [file...]" << "\n\n";

        std::cout << "\033[1;33m" << "OPTIONS" << "\033[0m" << "\n";
        std::cout << "  \033[1;32m" << "--renderer=<backend>" << "\033[0m"
                  << "   Select graphics backend\n";
        std::cout << "                  \033[1;37m" << "vulkan" << "\033[0m"
                  << "       Vulkan (default on Linux)\n";
        std::cout << "                  \033[1;37m" << "opengl" << "\033[0m"
                  << "      OpenGL (default fallback)\n";
        std::cout << "                  \033[1;37m" << "d3d11" << "\033[0m"
                  << "       Direct3D 11 (default on Windows)\n";
        std::cout << "                  \033[1;37m" << "null" << "\033[0m"
                  << "         No-op renderer\n\n";

        std::cout << "  \033[1;32m" << "--gpu=<index>" << "\033[0m"
                  << "        Select GPU by index (0-based)\n\n";

        std::cout << "  \033[1;32m" << "-h, --help" << "\033[0m"
                  << "         Show this help message\n\n";

        std::cout << "\033[1;33m" << "EXAMPLES" << "\033[0m" << "\n";
        std::cout << "  OIViewer image.png" << "\n";
        std::cout << "  OIViewer --renderer=vulkan image.png" << "\n";
        std::cout << "  OIViewer --renderer=opengl" << "\n\n";
    }
}  // namespace

LLUtils::native_string_type CompileFilePathFromArguments(int argc, const LLUtils::native_char_type* const* argv)
{
    LLUtils::native_string_type filePath;
    bool showHelp = false;

    for (int index = 1; index < argc; ++index)
    {
        LLUtils::native_string_type arg = argv[index];
        LLUtils::native_string_type lowerArg;
        if (!arg.empty() && arg.front() == static_cast<LLUtils::native_char_type>('-'))
        {
            lowerArg = arg;
            std::transform(lowerArg.begin(), lowerArg.end(), lowerArg.begin(), ToLowerAscii);
        }

        if (lowerArg == LLUTILS_TEXT("--help") || lowerArg == LLUTILS_TEXT("-h"))
        {
            showHelp = true;
        }
        else if (lowerArg.starts_with(LLUTILS_TEXT("--renderer=")))
        {
            const std::string preferredRenderer = LLUtils::StringUtility::ConvertString<std::string>(
                lowerArg.substr(11));
            if (preferredRenderer.empty())
                throw std::invalid_argument("--renderer requires a backend name");
            OIV::OIV::SetPreferredRenderer(preferredRenderer.c_str());
        }
        else if (lowerArg.starts_with(LLUTILS_TEXT("--gpu=")))
        {
            gPreferredGPUIndex = ParseGpuIndex(arg.substr(6));
        }
        else
        {
            if (!filePath.empty())
                filePath += LLUTILS_TEXT(" ");
            filePath += arg;
        }
    }

    if (showHelp)
    {
        PrintHelp();
        std::exit(EXIT_SUCCESS);
    }

    return filePath;
}

int RunViewer(const LLUtils::native_string_type& filePath)
{
    OIV::OIV::SetPreferredGPUIndex(gPreferredGPUIndex);

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
