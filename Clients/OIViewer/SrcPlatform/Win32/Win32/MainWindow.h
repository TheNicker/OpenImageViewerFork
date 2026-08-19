#pragma once
#include "ImageControl.h"
#include <LWS/Cursor.hpp>
#include <LWS/Win32/WindowWin32.hpp>
#include <Windows.h>

namespace OIV
{
    namespace Win32
    {
        class MainWindow : public LWS::WindowWin32
        {
        public: // Types
            enum class CursorType
            {
                SystemDefault
                , Arrow
                , East
                , NorthEast
                , North
                , NorthWest
                , West
                , SouthWest
                , South
                , SouthEast
                , SizeAll
                , Count
            };

            MainWindow();
        public: // constant methods
            bool GetShowImageControl() const;
            bool GetShowStatusBar() const;
            SIZE GetCanvasSize() const;
            HWND GetCanvasHandle() const;
            HWND GetNativeHandle() const { return reinterpret_cast<HWND>(GetHandle()); }


        public: // mutating methods
            ImageControl& GetImageControl() { return fImageControl; }
            void SetCursorType(CursorType type);
            void ShowStatusBar(bool show);
            void SetShowImageControl(bool show);
            LWS::Window& GetCanvasWindow() { return fCanvasWindow; }
            void SetStatusBarText(LLUtils::native_string_type message, int part, int type);
            void SetIsTrayWindow(bool isTrayWindow);
            static bool GetIsTrayWindow(HWND hwnd);
            void SetDestoryOnClose(bool destroyOnClose) { SetDestroyOnClose(destroyOnClose); }
            void SetForground() { SetForeground(); }
            void SetPosition(int32_t x, int32_t y) { LWS::Window::SetPosition({x, y}); }
            void SetSize(uint32_t width, uint32_t height)
            {
                LWS::Window::SetSize({static_cast<int32_t>(width), static_cast<int32_t>(height)});
            }
            void SetWindowDisplayState(LWS::WindowDisplayState state) { SetDisplayState(state); }
            LWS::WindowDisplayState GetWindowDisplayState() const { return GetDisplayState(); }
            bool IsMouseCursorInClientRect() const { return IsMouseInClientRect(); }


        private: // methods
            void HandleResize();
            //void ResizeStatusBar();
            bool HandleWindwMessage(const LWS::AnyEvent& eventData);
            HWND DoCreateStatusBar(HWND hwndParent, uint32_t idStatus, HINSTANCE hinst, uint32_t cParts);
            void OnCreate();

        private: // member fields
            LWS::Window fCanvasWindow;
            HWND fHandleStatusBar = nullptr;
            //int fStatusWindowParts = 6;
            bool fShowStatusBar = true;
            bool fShowImageControl = false;
            CursorType fCurrentCursorType = CursorType::SystemDefault;
            std::array<LWS::Cursor, static_cast<int>(CursorType::Count)> fCursors{};
            bool fCursorsInitialized = false;
            ImageControl fImageControl;
        };
    }
}
