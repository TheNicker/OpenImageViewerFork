#include "ViewerApplication.h"

#include "ViewerApplicationPlatformState.h"

#include <LLUtils/Exception.h>

namespace OIV
{
    ViewerApplication::~ViewerApplication()
    {
        fIsShuttingDown = true;
        if (fCountingColorsThread.joinable())
            fCountingColorsThread.join();
    }

    void ViewerApplication::ShowSettings()
    {
        LL_EXCEPTION_NOT_IMPLEMENT("The settings window is not implemented on Linux");
    }
}  // namespace OIV
