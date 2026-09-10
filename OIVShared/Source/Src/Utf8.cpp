#include <OIVShared/Utf8.h>
#include <limits>
#include <stdexcept>
#include <type_traits>
#if LLUTILS_PLATFORM == LLUTILS_PLATFORM_WIN32
    #include <windows.h>
#endif

namespace OIV
{
    std::string EncodeUtf8(LLUtils::native_string_view text)
    {
#if LLUTILS_PLATFORM == LLUTILS_PLATFORM_WIN32
        static_assert(std::is_same_v<LLUtils::native_char_type, wchar_t>);
        std::string result;
        if (!text.empty())
        {
            if (text.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
                throw std::length_error("Native text exceeds the Windows conversion limit");
            const int count = static_cast<int>(text.size());
            const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), count, nullptr, 0, nullptr,
                                                 nullptr);
            if (size == 0)
                throw std::invalid_argument("Invalid UTF-16 renderer text");
            result.resize(static_cast<size_t>(size));
            if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), count, result.data(), size, nullptr,
                                    nullptr) != size)
                throw std::runtime_error("Could not encode renderer text as UTF-8");
        }
        return result;
#else
        return std::string(text);
#endif
    }

    LLUtils::native_string_type DecodeUtf8(std::string_view text)
    {
#if LLUTILS_PLATFORM == LLUTILS_PLATFORM_WIN32
        LLUtils::native_string_type result;
        if (!text.empty())
        {
            if (text.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
                throw std::length_error("UTF-8 text exceeds the Windows conversion limit");
            const int count = static_cast<int>(text.size());
            const int size  = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), count, nullptr, 0);
            if (size == 0)
                throw std::invalid_argument("Invalid UTF-8 renderer text");
            result.resize(static_cast<size_t>(size));
            if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), count, result.data(), size) != size)
                throw std::runtime_error("Could not decode UTF-8 renderer text");
        }
        return result;
#else
        return std::string(text);
#endif
    }
}  // namespace OIV
