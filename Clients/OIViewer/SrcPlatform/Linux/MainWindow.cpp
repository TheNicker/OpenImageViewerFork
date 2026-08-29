#include "MainWindow.h"

#include <LWS/Platform.hpp>
#include <LLUtils/Exception.h>

namespace OIV
{
    struct MainWindow::NativeState
    {
    };

    MainWindow::MainWindow() : fNativeState(std::make_unique<NativeState>())
    {
        std::ignore = AddEventListener(
            [this](const LWS::AnyEvent& eventData) noexcept
            {
                try
                {
                    return HandleWindowEvent(eventData);
                }
                catch (...)
                {
                    return true;
                }
            });
    }

    MainWindow::~MainWindow() = default;

    bool MainWindow::UseMainWindowAsCanvas() const
    {
        return LWS::Platform::supports(LWS::Platform::Feature::ServerSideDecorations) ||
               LWS::Platform::supports(LWS::Platform::Feature::HostWindowFrame);
    }

    void MainWindow::SetApplicationIcon() {}
    void MainWindow::UpdateNativeStatusBar([[maybe_unused]] LWS::Size& canvasSize) {}

    void MainWindow::SetStatusBarText([[maybe_unused]] LLUtils::native_string_type message, [[maybe_unused]] int part,
                                      [[maybe_unused]] int type)
    {
    }

    bool MainWindow::HandleWindowEvent(const LWS::AnyEvent& eventData)
    {
        if (std::holds_alternative<LWS::EventResize>(eventData))
            UpdateLayout();
        else if (std::holds_alternative<LWS::EventWindowDestroyed>(eventData))
            LWS::Platform::requestQuit();
        return false;
    }

    void MainWindow::SetIsTrayWindow([[maybe_unused]] bool isTrayWindow) {}

    bool MainWindow::GetIsTrayWindow([[maybe_unused]] LWS::Handle windowHandle)
    {
        LL_EXCEPTION_NOT_IMPLEMENT("Tray-window properties are not implemented on Linux");
    }

    void MainWindow::SetMenuChar([[maybe_unused]] bool suppress) {}
}  // namespace OIV
