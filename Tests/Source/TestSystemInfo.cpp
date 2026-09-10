#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <OIVAppCore/MessageHelper.h>
#include <LLUtils/StringUtility.h>

TEST_CASE("System information formats adapter index and acceleration as adjacent rows", "[AppCore][system-info]")
{
    const int index         = GENERATE(-1, 0, 7);
    const auto acceleration = GENERATE(LLUTILS_TEXT("Hardware"), LLUTILS_TEXT("Software"), LLUTILS_TEXT("Unknown"));
    const auto text         = OIV::MessageHelper::CreateSystemInfoMessage(
        LLUTILS_TEXT("OIViewer"), LLUTILS_TEXT("1.2.3.4"), LLUTILS_TEXT("abcdef12"), LLUTILS_TEXT("Debug"),
        LLUTILS_TEXT("Vulkan"), LLUTILS_TEXT("Test GPU"), index, acceleration, LLUTILS_TEXT("1.3"), {},
        LLUTILS_TEXT("Test OS"), LLUTILS_TEXT("8 physical / 16 logical"));
    for (const auto label :
         {LLUTILS_TEXT("Application"), LLUTILS_TEXT("Version"), LLUTILS_TEXT("Build"), LLUTILS_TEXT("Commit"),
          LLUTILS_TEXT("API version"), LLUTILS_TEXT("Driver version"), LLUTILS_TEXT("Not reported")})
        CHECK(text.find(label) != LLUtils::native_string_type::npos);
    const auto adapter         = text.find(LLUTILS_TEXT("Adapter"));
    const auto adapterIndex    = text.find(LLUTILS_TEXT("Adapter index"));
    const auto accelerationRow = text.find(LLUTILS_TEXT("Acceleration"));
    REQUIRE(adapter != text.npos);
    REQUIRE(adapterIndex != text.npos);
    REQUIRE(accelerationRow != text.npos);
    CHECK(adapter < adapterIndex);
    CHECK(adapterIndex < accelerationRow);
    const auto indexRow = text.substr(adapterIndex, accelerationRow - adapterIndex);
    CHECK(indexRow.find(LLUTILS_TEXT("...")) != text.npos);
    const auto expected = index < 0 ? LLUtils::native_string_type(LLUTILS_TEXT("Not reported"))
                                    : LLUtils::StringUtility::ConvertString<LLUtils::native_string_type>(
                                          std::to_string(index));
    CHECK(indexRow.find(expected) != text.npos);
    CHECK(text.find(acceleration, accelerationRow) != text.npos);
    CHECK(text.find(LLUTILS_TEXT("Adapter index:")) == text.npos);
}
