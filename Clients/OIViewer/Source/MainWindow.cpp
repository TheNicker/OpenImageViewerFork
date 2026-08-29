#include "MainWindow.h"

#include <utility>

namespace OIV
{
    LWS::Result MainWindow::Create(const LWS::WindowConfig& config)
    {
        const LWS::Result result = LWS::Window::Create(config);
        if (result == LWS::Result::Success)
            OnCreate();
        return result;
    }

    void MainWindow::SetCursorType(CursorType type)
    {
        if (type == fCurrentCursorType || type < CursorType::SystemDefault || type >= CursorType::Count)
            return;

        if (!fCursorsInitialized)
        {
            fCursors[static_cast<size_t>(CursorType::SystemDefault)].setCursorShape(LWS::CursorShape::Arrow);
            fCursors[static_cast<size_t>(CursorType::Arrow)].setCursorShape(LWS::CursorShape::Arrow);
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
        SetMouseCursor(type == CursorType::SystemDefault ? nullptr : &fCursors[static_cast<size_t>(type)]);
    }

    void MainWindow::OnCreate()
    {
        fUseMainWindowAsCanvas = UseMainWindowAsCanvas();
        if (!fUseMainWindowAsCanvas)
        {
            const LWS::WindowConfig canvasConfig{
                .position = {0, 0},
                .styles   = LWS::WindowStyle::ChildWindow,
            };
            fCanvasWindow.SetParent(this);
            if (fCanvasWindow.Create(canvasConfig) != LWS::Result::Success)
                return;

            fCanvasWindow.SetTransparent(true);
        }
        SetApplicationIcon();
        UpdateLayout();
    }

    bool MainWindow::GetShowImageControl() const
    {
        return fShowImageControl;
    }

    bool MainWindow::GetShowStatusBar() const
    {
        return fShowStatusBar &&
               ((GetWindowStyles() & (LWS::WindowStyle::Caption | LWS::WindowStyle::CloseButton |
                                      LWS::WindowStyle::MinimizeButton | LWS::WindowStyle::MaximizeButton)) !=
                LWS::WindowStyle::NoStyle);
    }

    void MainWindow::UpdateLayout()
    {
        LWS::Size canvasSize             = GetClientSize();
        constexpr int32_t imageListWidth = 200;
        if (fShowImageControl)
            canvasSize.x -= imageListWidth;

        UpdateNativeStatusBar(canvasSize);
        if (!fUseMainWindowAsCanvas)
        {
            fCanvasWindow.SetPosition({0, 0});
            fCanvasWindow.SetSize(canvasSize);
        }

        if (fImageControl.GetHandle() != 0)
        {
            fImageControl.SetVisible(fShowImageControl);
            if (fShowImageControl)
            {
                fImageControl.SetPosition({canvasSize.x, 0});
                fImageControl.SetSize({imageListWidth, canvasSize.y});
            }
        }
    }

    void MainWindow::ShowStatusBar(bool show)
    {
        if (show != fShowStatusBar)
        {
            fShowStatusBar = show;
            UpdateLayout();
        }
    }

    void MainWindow::SetShowImageControl(bool show)
    {
        if (show == fShowImageControl)
            return;

        fShowImageControl = show;
        if (fImageControl.GetHandle() == 0)
        {
            const LWS::WindowConfig imageControlConfig{
                .position = {0, 0},
                .styles   = LWS::WindowStyle::ChildWindow,
            };
            fImageControl.SetParent(this);
            std::ignore = fImageControl.Create(imageControlConfig);
        }
        UpdateLayout();
    }

    LWS::Handle MainWindow::GetCanvasHandle() const
    {
        return fUseMainWindowAsCanvas ? GetHandle() : fCanvasWindow.GetHandle();
    }

    LWS::Handle MainWindow::GetNativeHandle() const
    {
        return GetHandle();
    }

    LWS::Size MainWindow::GetCanvasSize() const
    {
        return fUseMainWindowAsCanvas ? GetClientSize() : fCanvasWindow.GetClientSize();
    }

    ImageControl& MainWindow::GetImageControl()
    {
        return fImageControl;
    }

    LWS::Window& MainWindow::GetCanvasWindow()
    {
        return fUseMainWindowAsCanvas ? static_cast<LWS::Window&>(*this) : fCanvasWindow;
    }

    void MainWindow::ShowCanvas()
    {
        if (!fUseMainWindowAsCanvas)
            fCanvasWindow.SetVisible(true);
    }

    void MainWindow::SetDestoryOnClose(bool destroyOnClose)
    {
        SetDestroyOnClose(destroyOnClose);
    }

    void MainWindow::SetForground()
    {
        SetForeground();
    }

    void MainWindow::SetPosition(int32_t x, int32_t y)
    {
        LWS::Window::SetPosition({x, y});
    }

    void MainWindow::SetSize(uint32_t width, uint32_t height)
    {
        LWS::Window::SetSize({static_cast<int32_t>(width), static_cast<int32_t>(height)});
    }

    void MainWindow::SetWindowDisplayState(LWS::WindowDisplayState state)
    {
        SetDisplayState(state);
    }

    LWS::WindowDisplayState MainWindow::GetWindowDisplayState() const
    {
        return GetDisplayState();
    }

    bool MainWindow::IsMouseCursorInClientRect() const
    {
        return IsMouseInClientRect();
    }
}  // namespace OIV
