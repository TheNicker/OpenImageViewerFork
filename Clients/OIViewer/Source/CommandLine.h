#pragma once

#include <Interfaces/RendererOptions.h>
#include <LLUtils/StringDefs.h>
#include <variant>

namespace OIV
{
    struct CommandLineParameters
    {
        std::optional<LLUtils::native_string_type> inputPath;
        RendererOptions rendering;
    };

    struct CommandLineExit
    {
        int exitCode = 0;
        std::string standardOutput;
        std::string standardError;
    };

    using CommandLineParseResult = std::variant<CommandLineParameters, CommandLineExit>;
    CommandLineParseResult ParseCommandLine(int argc, const LLUtils::native_char_type* const* argv);
    bool ShouldForwardInput(const CommandLineParameters& parameters);
}  // namespace OIV
