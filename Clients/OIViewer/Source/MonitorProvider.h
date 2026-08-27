#pragma once

#include "EventManager.h"

#include <LWS/Platform.hpp>

namespace OIV
{
    // Transitional shim until LWS reports monitor changes for a window directly.
    class MonitorProvider
    {
      public:

        void UpdateFromWindowHandle(LWS::Handle windowHandle);

      private:

        LWS::Platform::MonitorDesc fMonitorDesc{};
    };
}  // namespace OIV
