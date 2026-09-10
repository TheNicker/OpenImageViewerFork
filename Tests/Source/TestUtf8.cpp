#include <catch2/catch_test_macros.hpp>
#include <OIVShared/Utf8.h>
#include <clocale>
#include <stdexcept>

TEST_CASE("Renderer text preserves Unicode independently of the C locale", "[renderer][utf8]")
{
    struct LocaleRestore
    {
        std::string previous = std::setlocale(LC_CTYPE, nullptr);
        ~LocaleRestore() { std::setlocale(LC_CTYPE, previous.c_str()); }
    } restore;
    REQUIRE(std::setlocale(LC_CTYPE, "C") != nullptr);
    const LLUtils::native_string_type native = LLUTILS_TEXT("GPU \u00e9 \u05d0 \U0001f4f7");
    const std::string encoded                = "GPU \xc3\xa9 \xd7\x90 \xf0\x9f\x93\xb7";
    CHECK(OIV::EncodeUtf8(native) == encoded);
    CHECK(OIV::DecodeUtf8(encoded) == native);
    CHECK(OIV::EncodeUtf8({}).empty());
    CHECK(OIV::DecodeUtf8({}).empty());
#if LLUTILS_PLATFORM == LLUTILS_PLATFORM_WIN32
    CHECK_THROWS_AS(OIV::EncodeUtf8(std::wstring_view(L"\xd800", 1)), std::invalid_argument);
    CHECK_THROWS_AS(OIV::DecodeUtf8(std::string_view("\xff", 1)), std::invalid_argument);
#endif
}
