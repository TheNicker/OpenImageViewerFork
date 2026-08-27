#include "MainWindow.h"

#include "Resource.h"

#include <LWS/Win32/EventWin32.hpp>
#include <LWS/Win32/WindowBackendWin32.hpp>

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
        const auto* raw = std::get_if<LWS::EventRawPlatform>(&eventData);
        if (raw == nullptr || raw->platformType != std::to_underlying(LWS::BackendId::Win32) ||
            raw->platformData == nullptr)
            return false;

        const auto& message = *reinterpret_cast<const LWS::Win32::WinMessage*>(raw->platformData);
        switch (message.message)
        {
            case WM_SIZE:
                HandleResize();
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            case WM_ACTIVATE:
                if (message.wParam != WA_INACTIVE)
                    SetIsTrayWindow(false);
                break;
            default:
                break;
        }
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

    void MainWindow::SetMenuChar(bool suppress)
    {
        if (auto* backend = getBackendAs<LWS::WindowBackendWin32>())
            backend->setMenuChar(suppress);
    }
}  // namespace OIV
