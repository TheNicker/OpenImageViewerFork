#include "ViewerApplication.h"

#include "FileWatcherWin32.h"
#include "Resource.h"
#include "ViewerApplicationPlatformState.h"

#include <LLUtils/Exception.h>
#include <LLUtils/PlatformUtility.h>

#include <Windows.h>

#include <LWS/Win32/WindowExtensions.hpp>

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
        fNativeWindowState.reset(new NativeWindowState(fPlatform));
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
        const auto canvasHandle = LWS::Win32::GetHwnd(fWindow.GetCanvasWindow());
        if (!canvasHandle.has_value())
            LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Unable to obtain the canvas window handle");
        fRenderGateway->Initialize(reinterpret_cast<LWS::Handle>(*canvasHandle));
    }

    LWS::Rect ViewerApplication::GetNotificationIconRect(LWS::NotificationIconGroup::IconID iconId) const
    {
        return fNativeWindowState->notificationIcons.GetIconRect(iconId);
    }

    void ViewerApplication::Run()
    {
        fPlatform.RunMessageLoop();
    }
}  // namespace OIV
