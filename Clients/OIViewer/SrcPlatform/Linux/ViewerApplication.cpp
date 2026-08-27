#include "ViewerApplication.h"

#include "ViewerApplicationPlatformState.h"

#include <LLUtils/Exception.h>
#include <LLUtils/PlatformUtility.h>

#include <cstdlib>
#include <filesystem>

#ifdef LWS_PLATFORM_WAYLAND
    #include <LWS/Wayland/PlatformWayland.hpp>
#endif

namespace OIV
{
    LLUtils::native_string_type ViewerApplication::GetAppDataFolder()
    {
        const char* configHome = std::getenv("XDG_CONFIG_HOME");
        std::filesystem::path folder;
        if (configHome != nullptr && *configHome != '\0')
        {
            folder = configHome;
        }
        else if (const char* userHome = std::getenv("HOME"); userHome != nullptr && *userHome != '\0')
        {
            folder = std::filesystem::path(userHome) / ".config";
        }
        else
        {
            folder = std::filesystem::temp_directory_path();
        }
        return (folder / "OIV").native() + LLUTILS_TEXT("/");
    }

    LWS::Handle ViewerApplication::FindTrayBarWindow()
    {
        return 0;
    }

    LLUtils::native_string_type ViewerApplication::GetApplicationModulePath()
    {
        return LLUtils::PlatformUtility::GetExePath();
    }

    void ViewerApplication::InitializePlatformState()
    {
        fRawInputState.reset(new RawInputState());
        fNativeWindowState.reset(new NativeWindowState());
        fFileWatcher.reset();
        fRenderGateway = std::make_unique<OivRenderGateway>(OivRenderGateway::PresentationState::Deferred);
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
        fNotificationIconID = 0;
    }

    void ViewerApplication::InitializeRenderer()
    {
        void* nativeDisplay = nullptr;
#ifdef LWS_PLATFORM_WAYLAND
        nativeDisplay = LWS::Wayland::GetDisplay();
#endif
        fRenderGateway->Initialize(fWindow.GetCanvasWindow().GetHandle(), nativeDisplay);
    }

    LWS::Rect ViewerApplication::GetNotificationIconRect(
        [[maybe_unused]] LWS::NotificationIconGroup::IconID iconId) const
    {
        LL_EXCEPTION_NOT_IMPLEMENT("Notification icons are not implemented on Linux");
    }

    void ViewerApplication::Run()
    {
        LWS::Platform::runMessageLoop();
    }
}  // namespace OIV
