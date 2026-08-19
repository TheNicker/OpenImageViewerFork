#pragma once
#include <LWS/Platform.hpp>
#include <Windows.h>

namespace OIV
{
    class MonitorProvider
    {
    public:
        void UpdateFromWindowHandle(HWND hwnd)
        {
            HMONITOR hmonitor = MonitorFromWindow(hwnd, 0);
            if (reinterpret_cast<LWS::Handle>(hmonitor) != fMonitorDesc.handle) // update frame rate only if monitor has changed.
            {
                LWS::Platform::refreshMonitors(); // refresh in case monitors were added or removed since last refresh.
                fMonitorDesc = LWS::Platform::getMonitorInfo(reinterpret_cast<LWS::Handle>(hmonitor));
                //fMonitorProvider.SetCurrentMonitor(MonitorInfo::GetSingleton().getMonitorInfo(hmonitor));

                EventManager::MonitorChangeEventParams params = { fMonitorDesc };
                EventManager::GetSingleton().MonitorChange.Raise(params);
            }
        }

    private:
        LWS::Platform::MonitorDesc fMonitorDesc{};
    };
}
