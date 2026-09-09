#include "MonitorProvider.h"

#include <LWS/Window.hpp>

#include <Windows.h>

#include <LWS/Win32/WindowExtensions.hpp>

namespace OIV
{
    void MonitorProvider::UpdateFromWindow(LWS::Window& window)
    {
        const HMONITOR monitor        = MonitorFromWindow(*LWS::Win32::GetHwnd(window), 0);
        const uintptr_t monitorHandle = reinterpret_cast<uintptr_t>(monitor);
        if (monitorHandle != fMonitorDesc.handle)
        {
            auto& platform = window.GetPlatformContext();
            std::ignore    = platform.RefreshMonitors();
            fMonitorDesc   = *platform.GetMonitorInfo(monitorHandle);
            const EventManager::MonitorChangeEventParams args{.monitorDesc = fMonitorDesc};
            EventManager::GetSingleton().MonitorChange.Raise(args);
        }
    }
}  // namespace OIV
