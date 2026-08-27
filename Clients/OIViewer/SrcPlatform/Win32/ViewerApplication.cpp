#include "ViewerApplication.h"

#include "FileWatcherWin32.h"
#include "Resource.h"
#include "ViewerApplicationPlatformState.h"

#include <LLUtils/Exception.h>
#include <LLUtils/PlatformUtility.h>

#include <Windows.h>

#include <iostream>

namespace OIV
{
    LLUtils::native_string_type ViewerApplication::GetAppDataFolder()
    {
        return LLUtils::PlatformUtility::GetAppDataFolder() + LLUTILS_TEXT("/OIV/");
    }

    LWS::Handle ViewerApplication::FindTrayBarWindow()
    {
        HWND nextChild = nullptr;
        do
        {
            nextChild = FindWindowEx(nullptr, nextChild, nullptr, nullptr);
        } while (nextChild != nullptr && !MainWindow::GetIsTrayWindow(reinterpret_cast<LWS::Handle>(nextChild)));
        return reinterpret_cast<LWS::Handle>(nextChild);
    }

    LLUtils::native_string_type ViewerApplication::GetApplicationModulePath()
    {
        return LLUtils::PlatformUtility::GetDllPath();
    }

    void ViewerApplication::InitializePlatformState()
    {
        fRawInputState.reset(new RawInputState(*this));
        fNativeWindowState.reset(new NativeWindowState());
        fFileWatcher   = std::make_unique<Win32::FileWatcherWin32>();
        fRenderGateway = std::make_unique<OivRenderGateway>();
    }

    void ViewerApplication::RawInputStateDeleter::operator()(RawInputState* state) const noexcept
    {
        delete state;
    }

    void ViewerApplication::NativeWindowStateDeleter::operator()(NativeWindowState* state) const noexcept
    {
        delete state;
    }

    void ViewerApplication::InitializeNotificationIcons()
    {
        fNotificationIconID = fNativeWindowState->notificationIcons.AddIconResource(IDI_APP_ICON,
                                                                                    LLUTILS_TEXT("Open Image Viewer"));
        fNativeWindowState->notificationIcons.OnNotificationIconEvent.Add(
            std::bind(&ViewerApplication::OnNotificationIcon, this, std::placeholders::_1));
    }

    void ViewerApplication::InitializeRenderer()
    {
        fRenderGateway->Initialize(fWindow.GetCanvasWindow().GetHandle());
    }

    LWS::Rect ViewerApplication::GetNotificationIconRect(LWS::NotificationIconGroup::IconID iconId) const
    {
        return fNativeWindowState->notificationIcons.GetIconRect(iconId);
    }

    void ViewerApplication::Run()
    {
        bool shouldQuit = false;
        while (!shouldQuit)
        {
            constexpr DWORD count    = 1;
            const HANDLE eventHandle = reinterpret_cast<HANDLE>(fEventSync.GetEventHandle());
            const DWORD result       = MsgWaitForMultipleObjects(count, &eventHandle, FALSE, INFINITE, QS_ALLINPUT);
            if (result < count)
            {
                fEventSync.ProcessData();
            }
            else if (result == WAIT_FAILED)
            {
                std::cerr << "Wait failed! Error: " << GetLastError() << std::endl;
                shouldQuit = true;
            }
            else
            {
                shouldQuit = LWS::Platform::processMessages();
            }
        }
    }
}  // namespace OIV
