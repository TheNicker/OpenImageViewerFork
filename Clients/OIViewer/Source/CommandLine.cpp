#include "CommandLine.h"

#include <CLI/CLI.hpp>
#include <Version.h>
#include <sstream>
#include <vector>

namespace OIV
{
    namespace
    {
        void ConfigureCommandLine(CLI::App& app, CommandLineParameters& parameters,
                                  std::vector<LLUtils::native_string_type>& input)
        {
            std::string choices;
            for (const auto& info : GetBuiltRenderers())
            {
                if (!choices.empty())
                    choices += " -> ";
                choices += info.name;
            }
            app.add_option_function<std::string>(
                   "--renderer",
                   [&parameters](const std::string& value)
                   {
                       for (const auto& info : GetBuiltRenderers())
                           if (detail::AsciiEqual(value, info.name))
                               parameters.rendering.renderer = info.type;
                   },
                   "Rendering API: " + choices)
                ->check(
                    [](const std::string& value)
                    {
                        const bool built = std::ranges::any_of(GetBuiltRenderers(), [&](const auto& info)
                                                               { return detail::AsciiEqual(value, info.name); });
                        return built ? std::string{} : "Choose a built renderer: " + value + " is unavailable";
                    })
                ->type_name("NAME");
            app.add_option("--adapter_name", parameters.rendering.adapterName,
                           "Vendor or GPU-name substring; ignored when an index is supplied")
                ->check([](const std::string& value) { return value.empty() ? "Adapter name cannot be empty" : ""; });
            app.add_option("--adapter_index", parameters.rendering.adapterIndex,
                           "Nonnegative API-specific index; overrides the name and disables fallback")
                ->check(CLI::NonNegativeNumber);
            app.footer("Selection policy:\n"
                       "  Prefer hardware, then unknown acceleration, then software.\n"
                       "  API preference within each group: " +
                       (choices.empty() ? "none" : choices) +
                       ".\n"
                       "  --renderer fixes the API; no API fallback.\n"
                       "  --adapter_name matches vendor/name text; first usable match.\n"
                       "  --adapter_index overrides the name and disables fallback,\n"
                       "  using the selected API or the build's default API.");
            app.add_option("input", input, "Image or folder; unquoted path tokens are joined with spaces");
            app.set_version_flag("--version", FormatFullVersion(CurrentVersion));
        }
    }  // namespace

    CommandLineParseResult ParseCommandLine(int argc, const LLUtils::native_char_type* const* argv)
    {
        CLI::App app{"OpenImageViewer"};
        CommandLineParameters parameters;
        std::vector<LLUtils::native_string_type> input;
        ConfigureCommandLine(app, parameters, input);
        try
        {
            app.parse(argc, argv);
        }
        catch (const CLI::ParseError& error)
        {
            std::ostringstream out, err;
            const int code = app.exit(error, out, err);
            return CommandLineExit{code, out.str(), err.str()};
        }
        if (!input.empty())
        {
            parameters.inputPath.emplace(std::move(input.front()));
            for (size_t i = 1; i < input.size(); ++i)
                *parameters.inputPath += LLUTILS_TEXT(" ") + input[i];
        }
        return parameters;
    }

    bool ShouldForwardInput(const CommandLineParameters& parameters)
    {
        // Explicit graphics options must reach a new instance rather than an existing process's renderer.
        return parameters.inputPath.has_value() && !parameters.inputPath->empty() && !parameters.rendering.renderer &&
               !parameters.rendering.adapterName && !parameters.rendering.adapterIndex;
    }
}  // namespace OIV
