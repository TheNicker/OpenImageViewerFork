#include "ViewerApplication.h"

#include "ViewerApplicationPlatformState.h"

#include <LLUtils/Exception.h>
#include <LLUtils/PlatformUtility.h>

#include <cstdlib>
#include <filesystem>

namespace OIV
{
    namespace
    {
        class ViewerRenderPortLinux final : public IViewerRenderPort
        {
          public:

            void Initialize([[maybe_unused]] std::size_t canvasHandle) override {}
            ResultCode Refresh() override { return RC_NotImplemented; }

            void SetSelectionRect([[maybe_unused]] const LLUtils::RectI32& rect) override
            {
                LL_EXCEPTION_NOT_IMPLEMENT("Selection rendering is not implemented on Linux");
            }

            void ClearSelectionRect() override
            {
                LL_EXCEPTION_NOT_IMPLEMENT("Selection rendering is not implemented on Linux");
            }

            ResultCode SetColorExposure([[maybe_unused]] const OIV_CMD_ColorExposure_Request& exposure) override
            {
                LL_EXCEPTION_NOT_IMPLEMENT("Color correction rendering is not implemented on Linux");
            }

            ResultCode SetTexelGrid([[maybe_unused]] const CmdRequestTexelGrid& grid) override
            {
                LL_EXCEPTION_NOT_IMPLEMENT("Texel-grid rendering is not implemented on Linux");
            }

            ResultCode SetClientSize([[maybe_unused]] uint16_t width, [[maybe_unused]] uint16_t height) override
            {
                return RC_NotImplemented;
            }

            ResultCode RegisterCallbacks([[maybe_unused]] const OIV_CMD_RegisterCallbacks_Request& callbacks) override
            {
                return RC_NotImplemented;
            }
        };
    }  // namespace

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
        fRenderGateway = std::make_unique<ViewerRenderPortLinux>();
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

    void ViewerApplication::InitializeRenderer() {}

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
