#pragma once
#include <LLUtils/StringDefs.h>
#include <string_view>

namespace OIV
{
    // Native text is UTF-16 on Windows and UTF-8 on Linux. These conversions do not
    // consult the C locale; Windows rejects malformed input at the encoding boundary.
    std::string EncodeUtf8(LLUtils::native_string_view text);
    LLUtils::native_string_type DecodeUtf8(std::string_view text);
}  // namespace OIV
