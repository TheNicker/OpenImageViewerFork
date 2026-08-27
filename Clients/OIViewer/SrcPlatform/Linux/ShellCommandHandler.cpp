#include "Helpers/ShellCommandHandler.h"

#include <LLUtils/Exception.h>

namespace OIV
{
    LLUtils::native_string_type ShellCommandHandler::Execute(
        [[maybe_unused]] const CommandManager::CommandRequest& request,
        [[maybe_unused]] const LLUtils::native_string_type& openedFileName,
        [[maybe_unused]] OIVBaseImageSharedPtr openedImage)
    {
        LL_EXCEPTION_NOT_IMPLEMENT("Shell commands are not implemented on Linux");
    }
}  // namespace OIV
