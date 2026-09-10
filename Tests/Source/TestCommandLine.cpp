#include <catch2/catch_test_macros.hpp>
#include <CommandLine.h>
#include <Version.h>
#include <vector>

namespace
{
    OIV::CommandLineParseResult Parse(std::initializer_list<const LLUtils::native_char_type*> args)
    {
        std::vector<const LLUtils::native_char_type*> argv{LLUTILS_TEXT("OIViewer")};
        argv.insert(argv.end(), args);
        return OIV::ParseCommandLine(static_cast<int>(argv.size()), argv.data());
    }
}  // namespace

TEST_CASE("CLI preserves native paths and explicit option boundaries", "[cli]")
{
    const auto empty = std::get<OIV::CommandLineParameters>(Parse({}));
    CHECK_FALSE(empty.inputPath);
    CHECK_FALSE(empty.rendering.renderer);
    CHECK_FALSE(empty.rendering.adapterName);
    CHECK_FALSE(empty.rendering.adapterIndex);
    CHECK_FALSE(OIV::ShouldForwardInput({.inputPath = LLUTILS_TEXT("")}));
    const auto path = std::get<OIV::CommandLineParameters>(Parse({LLUTILS_TEXT("folder"), LLUTILS_TEXT("image.png")}));
    CHECK(path.inputPath == LLUTILS_TEXT("folder image.png"));
    CHECK(OIV::ShouldForwardInput(path));
    const auto native = std::get<OIV::CommandLineParameters>(
        Parse({LLUTILS_TEXT("--"), LLUTILS_TEXT("--\u05ea\u05de\u05d5\u05e0\u05d4 \u7a7a.png")}));
    CHECK(native.inputPath == LLUTILS_TEXT("--\u05ea\u05de\u05d5\u05e0\u05d4 \u7a7a.png"));
}

TEST_CASE("CLI parses typed renderer and adapter selections", "[cli]")
{
    for (const auto& [name, type] : std::array{std::pair{LLUTILS_TEXT("gL"), OIV::RendererType::OpenGL},
                                               std::pair{LLUTILS_TEXT("d3D11"), OIV::RendererType::D3D11},
                                               std::pair{LLUTILS_TEXT("vUlKaN"), OIV::RendererType::Vulkan}})
    {
        const auto result = Parse({LLUTILS_TEXT("--renderer"), name});
        if (OIV::IsRendererAvailable(type))
        {
            REQUIRE(std::holds_alternative<OIV::CommandLineParameters>(result));
            CHECK(std::get<OIV::CommandLineParameters>(result).rendering.renderer == type);
        }
        else
            CHECK(std::get<OIV::CommandLineExit>(result).exitCode != 0);
    }
    const auto options = std::get<OIV::CommandLineParameters>(
        Parse({LLUTILS_TEXT("--adapter_name=Nvidia"), LLUTILS_TEXT("--adapter_index=0"),
               LLUTILS_TEXT("image with spaces.png")}));
    CHECK(options.rendering.adapterName == "Nvidia");
    CHECK(options.rendering.adapterIndex == 0);
    CHECK_FALSE(OIV::ShouldForwardInput(options));
    const auto named = std::get<OIV::CommandLineParameters>(
        Parse({LLUTILS_TEXT("--adapter_name=Intel(R) Graphics"), LLUTILS_TEXT("file.png")}));
    CHECK(named.rendering.adapterName == "Intel(R) Graphics");
    CHECK_FALSE(OIV::ShouldForwardInput(named));
    CHECK_FALSE(OIV::ShouldForwardInput(
        std::get<OIV::CommandLineParameters>(Parse({LLUTILS_TEXT("--adapter_index=0"), LLUTILS_TEXT("file.png")}))));
}

TEST_CASE("CLI rejects invalid missing duplicate and deprecated options", "[cli]")
{
    const std::vector<std::vector<const LLUtils::native_char_type*>> cases{
        {LLUTILS_TEXT("--renderer")},
        {LLUTILS_TEXT("--renderer=missing")},
        {LLUTILS_TEXT("--renderer=D3D12")},
        {LLUTILS_TEXT("--renderer=0")},
        {LLUTILS_TEXT("--renderer=2")},
        {LLUTILS_TEXT("--renderer=GL"), LLUTILS_TEXT("--renderer=Vulkan")},
        {LLUTILS_TEXT("--adapter_name")},
        {LLUTILS_TEXT("--adapter_name=")},
        {LLUTILS_TEXT("--adapter_name=A"), LLUTILS_TEXT("--adapter_name=B")},
        {LLUTILS_TEXT("--adapter_index")},
        {LLUTILS_TEXT("--adapter_index=-1")},
        {LLUTILS_TEXT("--adapter_index=1x")},
        {LLUTILS_TEXT("--adapter_index=99999999999999999999")},
        {LLUTILS_TEXT("--adapter_index=0"), LLUTILS_TEXT("--adapter_index=1")},
        {LLUTILS_TEXT("--adapter_name=Intel"), LLUTILS_TEXT("--adapter_index=-1")},
        {LLUTILS_TEXT("--adapter_name=Intel"), LLUTILS_TEXT("--adapter_index=bad")},
        {LLUTILS_TEXT("--renderer=OpenGL")},
        {LLUTILS_TEXT("--renderer=Null")},
        {LLUTILS_TEXT("--adapter=Intel")},
        {LLUTILS_TEXT("--unknown")},
        {LLUTILS_TEXT("--gpu=0")}};
    for (auto args : cases)
    {
        args.insert(args.begin(), LLUTILS_TEXT("OIViewer"));
        const auto parsed = OIV::ParseCommandLine(static_cast<int>(args.size()), args.data());
        REQUIRE(std::holds_alternative<OIV::CommandLineExit>(parsed));
        const auto& result = std::get<OIV::CommandLineExit>(parsed);
        CHECK(result.exitCode != 0);
        CHECK_FALSE(result.standardError.empty());
    }
}

TEST_CASE("CLI help and version are terminal successful results", "[cli]")
{
    for (const auto option : {LLUTILS_TEXT("-h"), LLUTILS_TEXT("--help"), LLUTILS_TEXT("--version")})
    {
        const auto result = std::get<OIV::CommandLineExit>(Parse({option}));
        CHECK(result.exitCode == 0);
        CHECK_FALSE(result.standardOutput.empty());
        CHECK(result.standardError.empty());
    }
    CHECK(std::get<OIV::CommandLineExit>(Parse({LLUTILS_TEXT("--version")}))
              .standardOutput.find(OIV::FormatFullVersion(OIV::CurrentVersion)) != std::string::npos);
}

TEST_CASE("Renderer validation reflects builds and never ignores explicit adapters", "[cli][renderer]")
{
    CHECK(OIV::IsRendererAvailable(OIV::GetDefaultRenderer()));
    for (const auto type : {OIV::RendererType::OpenGL, OIV::RendererType::D3D11, OIV::RendererType::Vulkan})
        CHECK(OIV::ValidateRendererOptions({.renderer = type}).empty() == OIV::IsRendererAvailable(type));
    CHECK_FALSE(OIV::ValidateRendererOptions({.renderer = static_cast<OIV::RendererType>(-1)}).empty());
    CHECK_FALSE(OIV::ValidateRendererOptions({.renderer = OIV::RendererType::OpenGL, .adapterIndex = 0}).empty());
    CHECK(OIV::ValidateRendererOptions({.adapterName = "GPU", .adapterIndex = 0}) ==
          OIV::ValidateRendererOptions({.adapterIndex = 0}));
    CHECK_FALSE(OIV::ValidateRendererOptions({.adapterName = " \t "}).empty());
    CHECK_FALSE(OIV::ValidateRendererOptions({.adapterIndex = -1}).empty());
    const bool supportsSelection = std::ranges::any_of(OIV::GetBuiltRenderers(),
                                                       [](const auto& info) { return info.supportsAdapterSelection; });
    CHECK(OIV::ValidateRendererOptions({.adapterName = "GPU"}).empty() == supportsSelection);
}

TEST_CASE("CLI help advertises only compiled APIs in compiled order", "[cli][renderer]")
{
    const auto help = std::get<OIV::CommandLineExit>(Parse({LLUTILS_TEXT("--help")})).standardOutput;
    std::string order;
    for (const auto& info : OIV::GetBuiltRenderers())
    {
        if (!order.empty())
            order += " -> ";
        order += info.name;
    }
    CHECK(help.find("API preference within each group: " + (order.empty() ? "none" : order) + ".") !=
          std::string::npos);
    for (const auto& [name, type] :
         std::array{std::pair{"GL", OIV::RendererType::OpenGL}, std::pair{"D3D11", OIV::RendererType::D3D11},
                    std::pair{"Vulkan", OIV::RendererType::Vulkan}})
        CHECK((help.find(name) != std::string::npos) == OIV::IsRendererAvailable(type));
    CHECK(help.find("OpenGL") == std::string::npos);
    CHECK(help.find("--adapter ") == std::string::npos);
}
