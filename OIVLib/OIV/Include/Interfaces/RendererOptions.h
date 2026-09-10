#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

namespace OIV
{
    enum class RendererType
    {
        OpenGL,
        D3D11,
        Vulkan,
        Null
    };

    struct RendererOptions
    {
        std::optional<RendererType> renderer;
        std::optional<std::string> adapter;
        std::optional<int> adapterIndex;
    };

    RendererType GetDefaultRenderer();
    bool IsRendererAvailable(RendererType renderer);
    // Returns an empty string for valid options; this does not initialize graphics or a window system.
    std::string ValidateRendererOptions(const RendererOptions& options);

    namespace detail
    {
        // Adapter names are UTF-8; fold ASCII vendor/model letters without locale-dependent conversions.
        inline bool AdapterNameMatches(std::string_view requested, std::string_view available)
        {
            const auto lower = [](unsigned char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
            return std::ranges::equal(requested, available, [&](char a, char b) { return lower(a) == lower(b); });
        }
    }  // namespace detail
}  // namespace OIV
