#pragma once
#include <cstdint>
namespace OIV
{
    enum class InterThreadMessages : uint16_t
    {
        FileChanged,
        AutoScroll,
        FirstFrameDisplayed,
        LoadFileExternally,
        CountColors
    };
}