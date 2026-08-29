#pragma once

#include "LinuxKeyBindings.h"
#include "ViewerApplication.h"

namespace OIV
{
    struct ViewerApplication::RawInputState
    {
        LinuxKeyBindings keyBindings;
    };

    struct ViewerApplication::NativeWindowState
    {
    };
}  // namespace OIV
