#include "CommandLine.h"

#include <CLI/CLI.hpp>
#include <Version.h>
#include <map>
#include <sstream>
#include <vector>

namespace OIV
{
    namespace
    {
        void ConfigureCommandLine(CLI::App& app, CommandLineParameters& parameters,
                                  std::vector<LLUtils::native_string_type>& input)
        {
            const std::map<std::string, RendererType> renderers{{"GL", RendererType::OpenGL},
                                                                {"OpenGL", RendererType::OpenGL},
                                                                {"D3D11", RendererType::D3D11},
                                                                {"Vulkan", RendererType::Vulkan}};
            app.add_option("--renderer", parameters.rendering.renderer, "Rendering backend: GL, D3D11 or Vulkan")
                ->transform(CLI::CheckedTransformer(renderers, CLI::ignore_case).description(""))
                // Transforms run newest first: reject numeric enum values before converting accepted names.
                ->transform(CLI::IsMember(renderers, CLI::ignore_case))
                ->type_name("NAME");
            auto* adapter = app.add_option("--adapter", parameters.rendering.adapter,
                                           "Exact adapter name (case-insensitive; D3D11/Vulkan)");
            auto* index   = app.add_option("--adapter_index", parameters.rendering.adapterIndex,
                                           "Zero-based adapter index (D3D11/Vulkan)")
                                ->check(CLI::NonNegativeNumber);
            adapter->excludes(index);
            adapter->check([](const std::string& value)
                           { return value.empty() ? "Adapter name cannot be empty" : ""; });
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
               !parameters.rendering.adapter && !parameters.rendering.adapterIndex;
    }
}  // namespace OIV
