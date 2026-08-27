#include "MonitorProvider.h"

#include <Windows.h>

namespace OIV
{
    void MonitorProvider::UpdateFromWindowHandle(LWS::Handle windowHandle)
    {
        const HMONITOR monitor          = MonitorFromWindow(reinterpret_cast<HWND>(windowHandle), 0);
        const LWS::Handle monitorHandle = reinterpret_cast<LWS::Handle>(monitor);
        if (monitorHandle != fMonitorDesc.handle)
        {
            LWS::Platform::refreshMonitors();
            fMonitorDesc = LWS::Platform::getMonitorInfo(monitorHandle);
            const EventManager::MonitorChangeEventParams args{.monitorDesc = fMonitorDesc};
            EventManager::GetSingleton().MonitorChange.Raise(args);
        }
    }
}  // namespace OIV
