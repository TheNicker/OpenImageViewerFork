#pragma once

#include "EventManager.h"

#include <LWS/Platform.hpp>

namespace OIV
{
    // Transitional shim until LWS reports monitor changes for a window directly.
    class MonitorProvider
    {
      public:

        void UpdateFromWindow(LWS::Window& window);

      private:

        LWS::MonitorDesc fMonitorDesc{};
    };
}  // namespace OIV
