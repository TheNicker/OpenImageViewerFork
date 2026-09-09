#include "ExceptionHandler.h"
#include "Main.h"

#include <LLUtils/Exception.h>

#include <cstdlib>
#include <exception>

namespace
{
    class ExceptionRegistration final
    {
      public:

        ExceptionRegistration() { OIV::RegisterExceptionhandler(); }
        ~ExceptionRegistration() { OIV::RemoveExceptionHandler(); }
    };
}  // namespace

int main(int argc, char* argv[])
{
    const ExceptionRegistration exceptionRegistration;
    try
    {
        return RunViewer(CompileFilePathFromArguments(argc, argv));
    }
    catch (const LLUtils::Exception&)
    {
        return EXIT_FAILURE;
    }
    catch (const std::exception& exception)
    {
        LL_EXCEPTION_DONT_THROW(LLUtils::Exception::ErrorCode::BadParameters, exception.what());
        return EXIT_FAILURE;
    }
    catch (...)
    {
        LL_EXCEPTION_DONT_THROW(LLUtils::Exception::ErrorCode::Unknown, "Unhandled entry-point exception");
        return EXIT_FAILURE;
    }
}
