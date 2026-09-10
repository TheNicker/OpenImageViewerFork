#include <catch2/catch_test_macros.hpp>
#include <OIVAppCore/MessageHelper.h>

TEST_CASE("System information separates renderer and adapter details", "[AppCore][system-info]")
{
    const auto text = OIV::MessageHelper::CreateSystemInfoMessage(LLUTILS_TEXT("OIViewer"), LLUTILS_TEXT("1.2.3.4"),
                                                                  LLUTILS_TEXT("abcdef12"), LLUTILS_TEXT("Debug"),
                                                                  LLUTILS_TEXT("Vulkan"), LLUTILS_TEXT("Test GPU"),
                                                                  LLUTILS_TEXT("1.3"), {}, LLUTILS_TEXT("Test OS"),
                                                                  LLUTILS_TEXT("8 physical / 16 logical"));
    for (const auto label : {LLUTILS_TEXT("Application"), LLUTILS_TEXT("Version"), LLUTILS_TEXT("Build"),
                             LLUTILS_TEXT("Commit"), LLUTILS_TEXT("Adapter"), LLUTILS_TEXT("API version"),
                             LLUTILS_TEXT("Driver version"), LLUTILS_TEXT("Not reported")})
        CHECK(text.find(label) != LLUtils::native_string_type::npos);
}
