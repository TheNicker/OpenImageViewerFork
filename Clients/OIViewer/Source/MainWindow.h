#pragma once

#include "ImageControl.h"

#include <LWS/Cursor.hpp>
#include <LWS/Window.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace OIV
{
    class MainWindow : public LWS::Window
    {
      public:

        enum class CursorType
        {
            SystemDefault,
            Arrow,
            East,
            NorthEast,
            North,
            NorthWest,
            West,
            SouthWest,
            South,
            SouthEast,
            SizeAll,
            Count
        };

        MainWindow();
        ~MainWindow() override;

        [[nodiscard]] LWS::Result Create(const LWS::WindowConfig& config = {});

        [[nodiscard]] bool GetShowImageControl() const;
        [[nodiscard]] bool GetShowStatusBar() const;
        [[nodiscard]] LWS::Size GetCanvasSize() const;
        [[nodiscard]] LWS::Handle GetCanvasHandle() const;
        [[nodiscard]] LWS::Handle GetNativeHandle() const;

        ImageControl& GetImageControl();
        void SetCursorType(CursorType type);
        void ShowStatusBar(bool show);
        void SetShowImageControl(bool show);
        LWS::Window& GetCanvasWindow();
        void SetStatusBarText(LLUtils::native_string_type message, int part, int type);
        void SetIsTrayWindow(bool isTrayWindow);
        static bool GetIsTrayWindow(LWS::Handle windowHandle);
        void SetMenuChar(bool suppress);
        void SetDestoryOnClose(bool destroyOnClose);
        void SetForground();
        void SetPosition(int32_t x, int32_t y);
        void SetSize(uint32_t width, uint32_t height);
        void SetWindowDisplayState(LWS::WindowDisplayState state);
        [[nodiscard]] LWS::WindowDisplayState GetWindowDisplayState() const;
        [[nodiscard]] bool IsMouseCursorInClientRect() const;

      private:

        struct NativeState;

        void HandleResize();
        bool HandleWindowEvent(const LWS::AnyEvent& eventData);
        void OnCreate();
        void SetApplicationIcon();
        void UpdateNativeStatusBar(LWS::Size& canvasSize);

        LWS::Window fCanvasWindow;
        bool fShowStatusBar           = true;
        bool fShowImageControl        = false;
        CursorType fCurrentCursorType = CursorType::SystemDefault;
        std::array<LWS::Cursor, static_cast<size_t>(CursorType::Count)> fCursors{};
        bool fCursorsInitialized = false;
        ImageControl fImageControl;
        std::unique_ptr<NativeState> fNativeState;
    };
}  // namespace OIV
