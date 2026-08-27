#include "ExceptionHandler.h"
#include "Main.h"

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
    return RunViewer(CompileFilePathFromArguments(argc, argv));
}
