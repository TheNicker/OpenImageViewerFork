#include "MainWindow.h"

#include "Resource.h"

#include <Windows.h>

#include <LWS/Win32/WindowExtensions.hpp>

#include <algorithm>
#include <cmath>

namespace OIV
{

    struct MainWindow::NativeState
    {
        HWND statusBar = nullptr;
    };

    MainWindow::MainWindow(LWS::PlatformContext& platform)
        : fWindow(platform), fCanvasWindow(platform), fImageControl(platform),
          fNativeState(std::make_unique<NativeState>())
    {
        auto connection = fWindow.Listen(
            [this](const LWS::AnyEvent& eventData)
            { return HandleWindowEvent(eventData) ? LWS::EventResponse::Handled : LWS::EventResponse::Unhandled; });
        if (connection.has_value())
            fEventConnection = std::move(*connection);
    }

    MainWindow::~MainWindow() = default;

    bool MainWindow::UseMainWindowAsCanvas() const
    {
        return false;
    }

    int32_t MainWindow::GetImageControlClientWidth(int32_t layoutWidth) const
    {
        const HWND window = *LWS::Win32::GetHwnd(fImageControl.GetWindow());
        RECT windowRect{};
        RECT clientRect{};
        GetWindowRect(window, &windowRect);
        GetClientRect(window, &clientRect);
        const auto clientArea        = fImageControl.GetWindow().GetClientAreaSize();
        const double scale           = clientArea.has_value() ? clientArea->Scale().x : 1.0;
        const int32_t nonClientWidth = static_cast<int32_t>(
            std::lround(((windowRect.right - windowRect.left) - (clientRect.right - clientRect.left)) / scale));
        return std::max(layoutWidth - nonClientWidth, 1);
    }

    void MainWindow::PrepareImageControlLayout()
    {
        const HWND window = *LWS::Win32::GetHwnd(fImageControl.GetWindow());
        SetWindowLongPtrW(window, GWL_STYLE, GetWindowLongPtrW(window, GWL_STYLE) | WS_VSCROLL);
        SetWindowPos(window, nullptr, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        SCROLLINFO info{};
        info.cbSize = sizeof(info);
        info.fMask  = SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL;
        info.nPage  = 1;
        SetScrollInfo(window, SB_VERT, &info, FALSE);
    }

    void MainWindow::SetApplicationIcon()
    {
        const HICON icon  = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APP_ICON));
        const HWND window = *LWS::Win32::GetHwnd(fWindow);
        ::SendMessage(window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
        ::SendMessage(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
    }

    void MainWindow::UpdateNativeStatusBar(LWS::LogicalSize& canvasSize)
    {
        if (fNativeState->statusBar == nullptr)
            return;

        const bool visible = GetShowStatusBar() && fWindow.GetWindowMode() == LWS::WindowMode::Windowed;
        ShowWindow(fNativeState->statusBar, visible ? SW_SHOW : SW_HIDE);
        if (visible)
        {
            RECT statusBarRect{};
            GetWindowRect(fNativeState->statusBar, &statusBarRect);
            const auto clientArea = fWindow.GetClientAreaSize();
            const double scale    = clientArea.has_value() ? clientArea->Scale().y : 1.0;
            canvasSize.y -= static_cast<int32_t>(std::lround((statusBarRect.bottom - statusBarRect.top) / scale));
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
        if (std::holds_alternative<LWS::EventClientAreaSizeChanged>(eventData))
            UpdateLayout();
        return false;
    }

    void MainWindow::SetIsTrayWindow(bool isTrayWindow)
    {
        SetProp(*LWS::Win32::GetHwnd(fWindow), LLUTILS_TEXT("isTrayWindow"),
                isTrayWindow ? reinterpret_cast<HANDLE>(1) : nullptr);
    }

    bool MainWindow::GetIsTrayWindow(LWS::Handle windowHandle)
    {
        return GetProp(reinterpret_cast<HWND>(windowHandle), LLUTILS_TEXT("isTrayWindow")) != nullptr;
    }
}  // namespace OIV
