#include "MainWindow.h"
#include <LLUtils/Buffer.h>
#include <LWS/Win32/EventWin32.hpp>
#include "../Resource.h"
#include <variant>

namespace OIV
{
    namespace Win32
    {
        MainWindow::MainWindow()
        {
            AddEventListener([this](const LWS::AnyEvent& eventData) { return HandleWindwMessage(eventData); });
        }


        void MainWindow::SetCursorType(CursorType type)
        {
            if (type != fCurrentCursorType && type >= CursorType::SystemDefault)
            {
                fCurrentCursorType = type;

                if (fCursorsInitialized == false)
                {
                    fCursors[static_cast<size_t>(CursorType::SystemDefault)].setCursorShape(LWS::CursorShape::Arrow);
                    fCursors[static_cast<size_t>(CursorType::East)].setCursorShape(LWS::CursorShape::SizeEW);
                    fCursors[static_cast<size_t>(CursorType::NorthEast)].setCursorShape(LWS::CursorShape::SizeNESW);
                    fCursors[static_cast<size_t>(CursorType::North)].setCursorShape(LWS::CursorShape::SizeNS);
                    fCursors[static_cast<size_t>(CursorType::NorthWest)].setCursorShape(LWS::CursorShape::SizeNWSE);
                    fCursors[static_cast<size_t>(CursorType::West)].setCursorShape(LWS::CursorShape::SizeEW);
                    fCursors[static_cast<size_t>(CursorType::SouthWest)].setCursorShape(LWS::CursorShape::SizeNESW);
                    fCursors[static_cast<size_t>(CursorType::South)].setCursorShape(LWS::CursorShape::SizeNS);
                    fCursors[static_cast<size_t>(CursorType::SouthEast)].setCursorShape(LWS::CursorShape::SizeNWSE);
                    fCursors[static_cast<size_t>(CursorType::SizeAll)].setCursorShape(LWS::CursorShape::SizeAll);

                    fCursorsInitialized = true;
                }
                fCurrentCursorType = type;
                SetMouseCursor(fCurrentCursorType == CursorType::SystemDefault
                                   ? nullptr
                                   : &fCursors[static_cast<int>(fCurrentCursorType)]);
            }
        }

        void MainWindow::OnCreate()
        {
            //fHandleStatusBar = DoCreateStatusBar(GetHandle(), 12, GetModuleHandle(nullptr), 3);
            //ResizeStatusBar();

            fCanvasWindow.Create();
            fCanvasWindow.SetParent(this);
            fCanvasWindow.SetVisible(true);
            fCanvasWindow.SetTransparent(true);

          
            HICON icon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APP_ICON));
            SendMessage(GetNativeHandle(), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
            SendMessage(GetNativeHandle(), WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));

           
       

            //SetStatusBarText(LLUTILS_TEXT("pixel: "), 0, SBT_NOBORDERS);
            //SetStatusBarText(LLUTILS_TEXT("File: "), 1, 0);

        }



        HWND MainWindow::DoCreateStatusBar(HWND hwndParent, uint32_t idStatus, HINSTANCE hinst, [[maybe_unused]] uint32_t cParts)
        {
            HWND hwndStatus;

            // Create the status bar.
            hwndStatus = CreateWindowEx(
                0, // no extended styles
                STATUSCLASSNAME, // name of status bar class
                nullptr, // no text when first created
                SBARS_SIZEGRIP | // includes a sizing grip
                WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, // creates a visible child window
                0, 0, 0, 0, // ignores size and position
                hwndParent, // handle to parent window
                reinterpret_cast<HMENU>(static_cast<size_t>(idStatus)), // child window identifier
                hinst, // handle to application instance
                nullptr); // no window creation data


            return hwndStatus;
        }


        void MainWindow::SetStatusBarText(LLUtils::native_string_type message, int part, int type)
        {
            if (fHandleStatusBar != nullptr)
                ::SendMessage(fHandleStatusBar, SB_SETTEXT, MAKEWORD(part, type), reinterpret_cast<LPARAM>(message.c_str()));
        }


        //void MainWindow::ResizeStatusBar()
        //{
        //    if (fHandleStatusBar == nullptr)
        //        return;
        //    RECT rcClient;
        //    HLOCAL hloc;
        //    PINT paParts;
        //    int i, nWidth;
        //    if (GetClientRect(GetHandle(), &rcClient) == 0)
        //        return;


        //    SetWindowPos(fHandleStatusBar, nullptr, 0, 0, -1, -1, 0);
        //    // Allocate an array for holding the right edge coordinates.
        //    hloc = LocalAlloc(LHND, sizeof(int) * fStatusWindowParts);
        //    if (hloc == nullptr)
        //        LL_EXCEPTION(LLUtils::Exception::ErrorCode::SystemError, "Unable to allocate statusbar memory");

        //    paParts = reinterpret_cast<PINT>(LocalLock(hloc));

        //    // Calculate the right edge coordinate for each part, and
        //    // copy the coordinates to the array.
        //    nWidth = rcClient.right / fStatusWindowParts;
        //    int rightEdge = nWidth;
        //    for (i = 0; i < fStatusWindowParts; i++)
        //    {
        //        paParts[i] = rightEdge;
        //        rightEdge += nWidth;
        //    }

        //    // Tell the status bar to create the window parts.
        //    ::SendMessage(fHandleStatusBar, SB_SETPARTS, (WPARAM)fStatusWindowParts, (LPARAM)
        //        paParts);

        //    // Free the array, and return.
        //    LocalUnlock(hloc);
        //    LocalFree(hloc);
        //}



        bool MainWindow::GetShowImageControl() const
        {
            return fShowImageControl;
        }

        bool MainWindow::GetShowStatusBar() const
        {
            // show status bar if explicity not visible and caption is visible
            return fShowStatusBar == true &&
                ((GetWindowStyles() & (LWS::WindowStyle::Caption | LWS::WindowStyle::CloseButton | LWS::WindowStyle::MinimizeButton | LWS::WindowStyle::MaximizeButton)) != LWS::WindowStyle::NoStyle);
        }

        void MainWindow::HandleResize()
        {
            RECT rect;
            ::GetClientRect(GetNativeHandle(), &rect);
            SIZE clientSize;
            const int ImageListWidth = 200;
            const bool isImageControlVisible = GetShowImageControl();
            clientSize.cx = rect.right - rect.left - (isImageControlVisible ?  ImageListWidth : 0 );
            clientSize.cy = rect.bottom - rect.top;


            if (GetShowStatusBar() && GetFullScreenState() == LWS::FullScreenState::Windowed)
            {
                RECT statusBarRect;
                ShowWindow(fHandleStatusBar, SW_SHOW);
                GetWindowRect(fHandleStatusBar, &statusBarRect);
                clientSize.cy -= statusBarRect.bottom - statusBarRect.top;
//                ResizeStatusBar();
            }
            else
            {
                ShowWindow(fHandleStatusBar, SW_HIDE);
            }

            SetWindowPos(reinterpret_cast<HWND>(fCanvasWindow.GetHandle()), nullptr, 0, 0, clientSize.cx, clientSize.cy, 0);

            if (fImageControl.GetHandle() != 0)
            {
                fImageControl.SetVisible(isImageControlVisible);

                if (isImageControlVisible)
                    SetWindowPos(reinterpret_cast<HWND>(fImageControl.GetHandle()), nullptr, clientSize.cx, 0, ImageListWidth, clientSize.cy, SWP_NOACTIVATE | SWP_NOZORDER);
            }

            ShowWindow(fHandleStatusBar, GetFullScreenState() == LWS::FullScreenState::Windowed ? SW_SHOW : SW_HIDE);
        }

        void MainWindow::ShowStatusBar(bool show)
        {
            if (show != fShowStatusBar)
            {
                fShowStatusBar = show;
                HandleResize();
            }
        }

        void MainWindow::SetShowImageControl(bool show)
        {
            if (show != fShowImageControl)
            {
                fShowImageControl = show;
                if (fImageControl.GetHandle() == 0)
				{
                    fImageControl.Create();
		            fImageControl.SetParent(this);
				}
                HandleResize();
            }
        }


        HWND MainWindow::GetCanvasHandle() const
        {
            return reinterpret_cast<HWND>(fCanvasWindow.GetHandle());
        }



        SIZE MainWindow::GetCanvasSize() const
        {
            RECT rect;
            ::GetClientRect(GetCanvasHandle(), &rect);
            return {rect.right - rect.left, rect.bottom - rect.top};
        }



        bool MainWindow::HandleWindwMessage(const LWS::AnyEvent& eventData)
        {
            const auto* raw = std::get_if<LWS::EventRawPlatform>(&eventData);
            if (raw == nullptr || raw->platformType != std::to_underlying(LWS::BackendId::Win32) ||
                raw->platformData == nullptr)
                return false;

            const LWS::Win32::WinMessage& message = *reinterpret_cast<const LWS::Win32::WinMessage*>(raw->platformData);

            switch (message.message)
            {
            case WM_CREATE:
                OnCreate();
                break;

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
            }
            return false;
        }

        void MainWindow::SetIsTrayWindow(bool isTrayWindow)
        {
            ::SetProp(GetNativeHandle(), LLUTILS_TEXT("isTrayWindow"), isTrayWindow ?  reinterpret_cast<HANDLE>(1) : nullptr);
        }
        
        bool MainWindow::GetIsTrayWindow(HWND hwnd)
        {
            return ::GetProp(hwnd, LLUTILS_TEXT("isTrayWindow")) != nullptr;
       }
    }
}
