#include "Main.h"

#include "ViewerApplication.h"

#include <LLUtils/Exception.h>
#include <LWS/Platform.hpp>

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
        const LWS::Platform::Session platformSession;
        if (!platformSession)
            throw std::runtime_error("Unable to initialize the LWS platform");

        OIV::ViewerApplication viewerApplication;
        viewerApplication.Init(filePath);
        viewerApplication.Run();
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
