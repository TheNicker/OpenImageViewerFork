#include "MainWindow.h"

#include "Resource.h"

#include <Windows.h>

namespace OIV
{
    struct MainWindow::NativeState
    {
        HWND statusBar = nullptr;
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
        return false;
    }

    void MainWindow::SetApplicationIcon()
    {
        const HICON icon  = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APP_ICON));
        const HWND window = reinterpret_cast<HWND>(GetHandle());
        ::SendMessage(window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
        ::SendMessage(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
    }

    void MainWindow::UpdateNativeStatusBar(LWS::Size& canvasSize)
    {
        if (fNativeState->statusBar == nullptr)
            return;

        const bool visible = GetShowStatusBar() && GetFullScreenState() == LWS::FullScreenState::Windowed;
        ShowWindow(fNativeState->statusBar, visible ? SW_SHOW : SW_HIDE);
        if (visible)
        {
            RECT statusBarRect{};
            GetWindowRect(fNativeState->statusBar, &statusBarRect);
            canvasSize.y -= statusBarRect.bottom - statusBarRect.top;
        }
    }

    void MainWindow::SetStatusBarText(LLUtils::native_string_type message, int part, int type)
    {
        if (fNativeState->statusBar != nullptr)
            ::SendMessage(fNativeState->statusBar, SB_SETTEXT, MAKEWORD(part, type),
                          reinterpret_cast<LPARAM>(message.c_str()));
    }

    bool MainWindow::HandleWindowEvent(const LWS::AnyEvent& eventData)
    {
        if (std::holds_alternative<LWS::EventResize>(eventData))
            UpdateLayout();
        return false;
    }

    void MainWindow::SetIsTrayWindow(bool isTrayWindow)
    {
        SetProp(reinterpret_cast<HWND>(GetHandle()), LLUTILS_TEXT("isTrayWindow"),
                isTrayWindow ? reinterpret_cast<HANDLE>(1) : nullptr);
    }

    bool MainWindow::GetIsTrayWindow(LWS::Handle windowHandle)
    {
        return GetProp(reinterpret_cast<HWND>(windowHandle), LLUTILS_TEXT("isTrayWindow")) != nullptr;
    }
}  // namespace OIV
