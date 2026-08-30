#include "ViewerMouseInput.h"

#include "ViewerApplication.h"
#include "OIVImage/OIVFileImage.h"

#include <LLUtils/PlatformUtility.h>
#include <LLUtils/StringUtility.h>

namespace OIV
{
    ViewerMouseInput::ViewerMouseInput(ViewerApplication& owner) : fOwner(owner)
    {
        fMultiClick.OnMouseClickEvent.Add([this](const auto& event) { OnMultiClick(event); });
    }

    const ViewerMouseInput::ButtonState* ViewerMouseInput::FindDevice(uint8_t deviceId) const
    {
        const auto device = fDevices.find(deviceId);
        return device == fDevices.end() ? nullptr : &device->second;
    }

    void ViewerMouseInput::SetButton(uint8_t deviceId, LWS::MouseButton button, bool pressed, bool mouseInside)
    {
        auto& state        = fDevices[deviceId];
        const size_t index = static_cast<size_t>(button);
        if (state[index] == pressed)
            return;

        state[index] = pressed;
        fCapture.Update(button, pressed, pressed && mouseInside);
        fMultiClick.SetButtonState(button, pressed);
        OnButton(deviceId, button, pressed, mouseInside);
    }

    void ViewerMouseInput::OnButton(uint8_t deviceId, LWS::MouseButton button, bool pressed, bool mouseInside)
    {
        const ButtonState* state = FindDevice(deviceId);
        if (state == nullptr)
            return;

        auto& canvas             = fOwner.fWindow.GetCanvasWindow();
        const bool leftDown      = state->at(static_cast<size_t>(LWS::MouseButton::Left));
        const bool rightDown     = state->at(static_cast<size_t>(LWS::MouseButton::Right));
        const bool rightCaptured = fCapture.IsCaptured(LWS::MouseButton::Right);

        if (button == LWS::MouseButton::Middle && pressed && mouseInside)
        {
            fOwner.fAutoScroll->ToggleAutoScroll();
            if (!fOwner.fAutoScroll->IsAutoScrolling())
            {
                fOwner.fWindow.SetCursorType(MainWindow::CursorType::SystemDefault);
                fOwner.fAutoScrollAnchor.reset();
            }
            else
            {
                const auto path = LLUtils::StringUtility::ToNativeString(LLUtils::PlatformUtility::GetExeFolder()) +
                                  LLUTILS_TEXT("./Resources/Cursors/ArrowC.cur");
                auto anchor     = std::make_unique<OIVFileImage>(path);
                if (anchor->Load(&fOwner.fImageLoader, IMCodec::PluginTraverseMode::AnyPlugin) == RC_Success)
                {
                    anchor->SetImageRenderMode(OIV_Image_Render_mode::IRM_Overlay);
                    anchor->SetPosition(static_cast<LLUtils::PointF64>(
                        static_cast<LLUtils::PointI32>(canvas.GetMousePosition()) -
                        static_cast<LLUtils::PointI32>(anchor->GetImage()->GetDimensions()) / 2));
                    anchor->SetScale({1.0, 1.0});
                    anchor->SetOpacity(0.5);
                    anchor->SetVisible(true);
                    fOwner.fAutoScrollAnchor = std::move(anchor);
                }
            }
        }

        if (button == LWS::MouseButton::Left)
        {
            if (!pressed)
                fOwner.fWindow.SetLockMouseToWindowMode(LWS::LockMouseToWindowMode::NoLock);

            if (pressed && rightDown && mouseInside)
            {
                fOwner.fRockerGestureActivate = true;
                fOwner.fContextMenuTimer.SetInterval(0);
                fOwner.JumpFiles(-1);
            }
            else if (!rightDown && !rightCaptured && !LWS::Platform::isKeyPressed(LWS::KeyCode::Alt) &&
                     !fOwner.fWindow.IsFullScreen())
            {
                const auto mode = LWS::Platform::isKeyPressed(LWS::KeyCode::Control)
                                      ? LWS::LockMouseToWindowMode::LockResize
                                      : LWS::LockMouseToWindowMode::LockMove;
                fOwner.fWindow.SetLockMouseToWindowMode(pressed ? mode : LWS::LockMouseToWindowMode::NoLock);
            }

            if (LWS::Platform::isKeyPressed(LWS::KeyCode::Alt))
            {
                SelectionRect::Operation operation = SelectionRect::Operation::NoOp;
                if (pressed && mouseInside)
                    operation = SelectionRect::Operation::BeginDrag;
                else if (!pressed && mouseInside)
                    operation = SelectionRect::Operation::EndDrag;
                fOwner.fSelectionRect.SetSelection(operation,
                                                   fOwner.SnapToScreenSpaceImagePixels(canvas.GetMousePosition()));
                fOwner.SaveImageSpaceSelection();
            }
        }

        if (button == LWS::MouseButton::X1 || button == LWS::MouseButton::X2)
        {
            if (pressed && mouseInside)
                fOwner.fTimerNavigation.SetInterval(fOwner.fQuickBrowseDelay);
            else if (!fCapture.IsCaptured(LWS::MouseButton::X1) && !fCapture.IsCaptured(LWS::MouseButton::X2))
                fOwner.fTimerNavigation.SetInterval(0);
        }

        if (button == LWS::MouseButton::Right)
        {
            if (pressed && mouseInside)
            {
                std::ignore     = fOwner.fWindow.GetCanvasWindow().SetPointerLocked(true);
                fRightDragDelta = {};
                if (leftDown)
                {
                    fOwner.fRockerGestureActivate = true;
                    fOwner.fContextMenuTimer.SetInterval(0);
                    fOwner.JumpFiles(1);
                }
                else if (fOwner.fContextMenu->IsSupported() && fOwner.fContextMenuTimer.GetInterval() == 0)
                {
                    fOwner.fContextMenuTimer.SetInterval(500);
                }
            }
            else if (!pressed)
            {
                std::ignore = fOwner.fWindow.GetCanvasWindow().SetPointerLocked(false);
                fOwner.fContextMenuTimer.SetInterval(0);
            }
        }
    }

    void ViewerMouseInput::Move(uint8_t deviceId, LWS::Point delta)
    {
        const ButtonState* state = FindDevice(deviceId);
        if (state == nullptr)
            return;
        const bool leftDown  = state->at(static_cast<size_t>(LWS::MouseButton::Left));
        const bool rightDown = state->at(static_cast<size_t>(LWS::MouseButton::Right));

        fMultiClick.SetMouseDelta(static_cast<int16_t>(delta.x), static_cast<int16_t>(delta.y));
        if (LWS::Platform::isKeyPressed(LWS::KeyCode::Alt) && fCapture.IsCaptured(LWS::MouseButton::Left))
        {
            fOwner.fSelectionRect.SetSelection(SelectionRect::Operation::Drag,
                                               fOwner.SnapToScreenSpaceImagePixels(
                                                   fOwner.fWindow.GetCanvasWindow().GetMousePosition()));
            fOwner.SaveImageSpaceSelection();
        }
        if (fCapture.IsCaptured(LWS::MouseButton::Right) && !fOwner.fContextMenu->IsVisible() && delta != LWS::Point{})
            fOwner.Pan(static_cast<LLUtils::PointF64>(delta));

        if (rightDown && fOwner.fContextMenuTimer.GetInterval() != 0)
        {
            fRightDragDelta += static_cast<LLUtils::Point<int64_t>>(delta);
            if (fRightDragDelta.x * fRightDragDelta.x + fRightDragDelta.y * fRightDragDelta.y > 25)
                fOwner.fContextMenuTimer.SetInterval(0);
        }
        else if (!rightDown)
            fOwner.fContextMenuTimer.SetInterval(0);

        if (!leftDown && !rightDown)
            fOwner.fRockerGestureActivate = false;
    }

    void ViewerMouseInput::Wheel(double steps)
    {
        if (steps == 0.0)
            return;

        auto& canvas             = fOwner.fWindow.GetCanvasWindow();
        const bool mouseInside   = canvas.IsMouseInClientRect();
        const bool rightCaptured = fCapture.IsCaptured(LWS::MouseButton::Right);
        if (mouseInside && LWS::Platform::isKeyPressed(LWS::KeyCode::Alt))
            fOwner.ExecutePredefinedCommand(steps > 0.0 ? "PreviousSubImage" : "NextSubImage");
        else if (mouseInside && LWS::Platform::isKeyPressed(LWS::KeyCode::Shift))
            fOwner.ExecutePredefinedCommand(steps > 0.0 ? "PreviousImageInFolder" : "NextImageInFolder");
        else if (rightCaptured || mouseInside)
        {
            const auto position = canvas.GetMousePosition();
            if (rightCaptured)
                fOwner.Zoom(steps * 0.2);
            else
                fOwner.Zoom(steps * 0.2, position.x, position.y);
        }
    }

    void ViewerMouseInput::OnMultiClick(const MouseMultiClickHandler::EventArgs& event)
    {
        auto& canvas = fOwner.fWindow.GetCanvasWindow();
        if (event.clickCount != 2 || !canvas.IsMouseInClientRect() || fDevices.empty())
            return;
        const auto& state    = fDevices.begin()->second;
        const bool leftDown  = state.at(static_cast<size_t>(LWS::MouseButton::Left));
        const bool rightDown = state.at(static_cast<size_t>(LWS::MouseButton::Right));
        if (event.button == LWS::MouseButton::Left && !rightDown)
        {
            if (fOwner.fSelectionRect.GetOperation() != SelectionRect::Operation::NoOp)
                fOwner.CancelSelection();
            else
                fOwner.ToggleFullScreen(LWS::Platform::isKeyPressed(LWS::KeyCode::Alt));
        }
        else if (event.button == LWS::MouseButton::Right && !leftDown)
            fOwner.ExecutePredefinedCommand("PasteImageFromClipboard");
    }

    void ViewerMouseInput::Cancel()
    {
        std::ignore = fOwner.fWindow.GetCanvasWindow().SetPointerLocked(false);
        fDevices.clear();
        fCapture.Reset();
        fMultiClick.Reset();
        fOwner.fContextMenuTimer.SetInterval(0);
        fOwner.fTimerNavigation.SetInterval(0);
        fOwner.fRockerGestureActivate = false;
    }

    int ViewerMouseInput::GetNavigationDirection() const
    {
        if (fDevices.empty())
            return 0;
        const auto& state = fDevices.begin()->second;
        if (state.at(static_cast<size_t>(LWS::MouseButton::X2)))
            return 1;
        return state.at(static_cast<size_t>(LWS::MouseButton::X1)) ? -1 : 0;
    }
}  // namespace OIV
