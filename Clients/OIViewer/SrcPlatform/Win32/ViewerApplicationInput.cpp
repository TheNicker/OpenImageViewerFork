#include "ViewerApplication.h"

#include "Globals.h"
#include "UserMessages.h"
#include "ViewerApplicationPlatformState.h"

#include <LInput/Keys/KeyCombination.h>
#include <LWS/Win32/EventWin32.hpp>

#include <LLUtils/Exception.h>
#include <LLUtils/PlatformUtility.h>
#include <LLUtils/StringUtility.h>

#include <OIVAppCore/ViewerPresentationPolicy.h>
#include <OIVAppCore/ConfigurationLoader.h>

#include <Windows.h>

namespace OIV
{
    namespace
    {
        const LWS::Win32::WinMessage* GetWinMessage(const LWS::AnyEvent& eventData)
        {
            const auto* raw = std::get_if<LWS::EventRawPlatform>(&eventData);
            if (raw == nullptr || raw->platformType != std::to_underlying(LWS::BackendId::Win32) ||
                raw->platformData == nullptr)
                return nullptr;
            return reinterpret_cast<const LWS::Win32::WinMessage*>(raw->platformData);
        }
    }  // namespace

    void ViewerApplication::RawInputState::OnMouseEvent(
        const LInput::ButtonStdExtension<MouseButtonType>::ButtonEvent& buttonEvent)
    {
        using namespace LInput;
        const bool mouseInside = owner.fWindow.IsUnderMouseCursor() && owner.fWindow.IsMouseCursorInClientRect();
        if (buttonEvent.button == MouseButton::Middle && buttonEvent.eventType == EventType::Pressed && mouseInside)
        {
            owner.fAutoScroll->ToggleAutoScroll();
            if (!owner.fAutoScroll->IsAutoScrolling())
            {
                owner.fWindow.SetCursorType(MainWindow::CursorType::SystemDefault);
                owner.fAutoScrollAnchor.reset();
            }
            else
            {
                const LLUtils::native_string_type anchorPath = LLUtils::StringUtility::ToNativeString(
                                                                   LLUtils::PlatformUtility::GetExeFolder()) +
                                                               LLUTILS_TEXT("./Resources/Cursors/ArrowC.cur");
                auto fileImage                               = std::make_unique<OIVFileImage>(anchorPath);
                if (fileImage->Load(&owner.fImageLoader, IMCodec::PluginTraverseMode::AnyPlugin) == RC_Success)
                {
                    fileImage->SetImageRenderMode(OIV_Image_Render_mode::IRM_Overlay);
                    fileImage->SetPosition(static_cast<LLUtils::PointF64>(
                        static_cast<LLUtils::PointI32>(owner.fWindow.GetMousePosition()) -
                        static_cast<LLUtils::PointI32>(fileImage->GetImage()->GetDimensions()) / 2));
                    fileImage->SetScale({1.0, 1.0});
                    fileImage->SetOpacity(0.5);
                    fileImage->SetVisible(true);
                    owner.fAutoScrollAnchor = std::move(fileImage);
                }
            }
        }

        if (buttonEvent.button == MouseButton::Left && buttonEvent.eventType == EventType::Released)
            owner.fWindow.SetLockMouseToWindowMode(LWS::LockMouseToWindowMode::NoLock);

        LWS::LockMouseToWindowMode lockMode = LWS::LockMouseToWindowMode::NoLock;
        const auto mouseStateIt             = mouseDevicesState.find(buttonEvent.parent->GetID());
        if (mouseStateIt == mouseDevicesState.end())
            return;

        const auto& mouseState   = mouseStateIt->second;
        const bool leftDown      = mouseState.GetButtonState(MouseButtonType::Left) == ButtonState::Down;
        const bool rightDown     = mouseState.GetButtonState(MouseButtonType::Right) == ButtonState::Down;
        const bool rightCaptured = mouseCaptureState.IsCaptured(MouseButtonType::Right);

        if (buttonEvent.button == MouseButton::Left)
        {
            if (buttonEvent.eventType == EventType::Pressed && rightDown && mouseInside)
            {
                owner.fRockerGestureActivate = true;
                owner.fContextMenuTimer.SetInterval(0);
                owner.JumpFiles(-1);
            }
            else if (!rightDown && !rightCaptured && !LWS::Platform::isKeyPressed(LWS::KeyCode::Alt) &&
                     !owner.fWindow.IsFullScreen())
            {
                lockMode = LWS::Platform::isKeyPressed(LWS::KeyCode::Control) ? LWS::LockMouseToWindowMode::LockResize
                                                                              : LWS::LockMouseToWindowMode::LockMove;
            }

            if (buttonEvent.eventType == EventType::Released)
                lockMode = LWS::LockMouseToWindowMode::NoLock;
            owner.fWindow.SetLockMouseToWindowMode(lockMode);

            if (LWS::Platform::isKeyPressed(LWS::KeyCode::Alt))
            {
                SelectionRect::Operation operation = SelectionRect::Operation::NoOp;
                if (buttonEvent.eventType == EventType::Pressed && owner.fWindow.IsUnderMouseCursor())
                    operation = SelectionRect::Operation::BeginDrag;
                else if (buttonEvent.eventType == EventType::Released && owner.fWindow.IsUnderMouseCursor())
                    operation = SelectionRect::Operation::EndDrag;
                owner.fSelectionRect.SetSelection(operation,
                                                  owner.SnapToScreenSpaceImagePixels(owner.fWindow.GetMousePosition()));
                owner.SaveImageSpaceSelection();
            }
        }

        if (buttonEvent.button == MouseButton::Back || buttonEvent.button == MouseButton::Forward)
        {
            if (buttonEvent.eventType == EventType::Pressed && owner.fWindow.IsUnderMouseCursor())
            {
                owner.fTimerNavigation.SetInterval(owner.fQuickBrowseDelay);
            }
            else if (!mouseCaptureState.IsCaptured(MouseButton::Back) &&
                     !mouseCaptureState.IsCaptured(MouseButton::Forward))
            {
                owner.fTimerNavigation.SetInterval(0);
            }
        }

        if (buttonEvent.button == MouseButton::Right && buttonEvent.eventType == EventType::Pressed && mouseInside)
        {
            if (leftDown)
            {
                owner.fRockerGestureActivate = true;
                owner.fContextMenuTimer.SetInterval(0);
                owner.JumpFiles(1);
            }

            if (owner.fContextMenuTimer.GetInterval() == 0 && !owner.fRockerGestureActivate)
            {
                owner.fContextMenuTimer.SetInterval(500);
                owner.fDownPosition = LWS::Platform::getMousePosition();
            }
        }
    }

    void ViewerApplication::RawInputState::OnMouseInput(const LInput::RawInput::RawInputEventMouse& mouseInput)
    {
        using namespace LInput;
        const auto mouseStateIt = mouseDevicesState.find(mouseInput.deviceIndex);
        if (mouseStateIt == mouseDevicesState.end())
            return;

        const auto& mouseState      = mouseStateIt->second;
        const bool leftDown         = mouseState.GetButtonState(MouseButtonType::Left) == ButtonState::Down;
        const bool rightDown        = mouseState.GetButtonState(MouseButtonType::Right) == ButtonState::Down;
        const bool rightCaptured    = mouseCaptureState.IsCaptured(MouseButtonType::Right);
        const bool leftCaptured     = mouseCaptureState.IsCaptured(MouseButtonType::Left);
        const bool mouseUnderWindow = owner.fWindow.IsUnderMouseCursor();

        if (LWS::Platform::isKeyPressed(LWS::KeyCode::Alt) && leftCaptured)
        {
            owner.fSelectionRect.SetSelection(SelectionRect::Operation::Drag,
                                              owner.SnapToScreenSpaceImagePixels(owner.fWindow.GetMousePosition()));
            owner.SaveImageSpaceSelection();
        }

        if (rightCaptured && !owner.fContextMenu->IsVisible() && (mouseInput.deltaX != 0 || mouseInput.deltaY != 0))
            owner.Pan(LLUtils::PointF64(mouseInput.deltaX, mouseInput.deltaY));

        const int32_t wheelDelta = mouseInput.wheelDelta;
        if (wheelDelta != 0)
        {
            if (mouseUnderWindow && LWS::Platform::isKeyPressed(LWS::KeyCode::Alt))
                owner.ExecutePredefinedCommand(wheelDelta > 0 ? "PreviousSubImage" : "NextSubImage");
            else if (mouseUnderWindow && LWS::Platform::isKeyPressed(LWS::KeyCode::Shift))
                owner.ExecutePredefinedCommand(wheelDelta > 0 ? "PreviousImageInFolder" : "NextImageInFolder");
            else if (rightCaptured || mouseUnderWindow)
            {
                const auto mousePosition = owner.fWindow.GetMousePosition();
                if (rightCaptured)
                    owner.Zoom(wheelDelta * 0.2);
                else
                    owner.Zoom(wheelDelta * 0.2, mousePosition.x, mousePosition.y);
            }
        }

        if (rightDown)
        {
            const LLUtils::PointI32 currentPosition = LWS::Platform::getMousePosition();
            if (currentPosition.DistanceSquared(owner.fDownPosition) > 25)
                owner.fContextMenuTimer.SetInterval(0);
        }
        else
        {
            owner.fContextMenuTimer.SetInterval(0);
        }

        if (!leftDown && !rightDown)
            owner.fRockerGestureActivate = false;
    }

    void ViewerApplication::RawInputState::OnRawInput(const LInput::RawInput::RawInputEvent& event)
    {
        using namespace LInput;
        if (event.deviceType != RawInput::RawInputDeviceType::Mouse)
            return;

        const auto& mouseEvent = static_cast<const RawInput::RawInputEventMouse&>(event);
        auto mouseStateIt      = mouseDevicesState.find(event.deviceIndex);
        if (mouseStateIt == mouseDevicesState.end())
        {
            mouseStateIt   = mouseDevicesState.emplace(event.deviceIndex, MouseButtonState{}).first;
            auto extension = std::make_shared<ButtonStdExtension<MouseButtonType>>(event.deviceIndex, 250, 0);
            extension->OnButtonEvent.Add(
                [this](const auto& buttonEvent)
                {
                    owner.HandleEventCallback(
                        [&]()
                        {
                            OnMouseEvent(buttonEvent);
                            return true;
                        });
                });
            mouseStateIt->second.AddExtension(
                std::static_pointer_cast<IButtonStateExtension<MouseButtonType>>(extension));
        }

        for (size_t index = 0; index < RawInput::MaxMouseButtons; ++index)
        {
            const auto button = static_cast<MouseButton>(index);
            mouseStateIt->second.SetButtonState(button, mouseEvent.buttonState[index]);
            const bool mouseUnderWindow = mouseEvent.buttonState[index] == ButtonState::Down &&
                                          owner.fWindow.IsUnderMouseCursor();
            mouseCaptureState.Update(button, mouseEvent.buttonState[index], mouseUnderWindow);
            mouseClickEventHandler.SetButtonState(button, mouseEvent.buttonState[index]);
        }

        mouseClickEventHandler.SetMouseDelta(mouseEvent.deltaX, mouseEvent.deltaY);
        OnMouseInput(mouseEvent);
    }

    void ViewerApplication::RawInputState::OnMouseMultiClick(const MouseMultiClickHandler::EventArgs& event)
    {
        using namespace LInput;
        if (event.clickCount != 2 || !owner.fWindow.IsMouseCursorInClientRect() ||
            !owner.fWindow.IsUnderMouseCursor() || mouseDevicesState.empty())
            return;

        const auto& mouseState = mouseDevicesState.begin()->second;
        const bool rightDown   = mouseState.GetButtonState(MouseButtonType::Right) == ButtonState::Down;
        const bool leftDown    = mouseState.GetButtonState(MouseButtonType::Left) == ButtonState::Down;
        if (event.button == MouseButton::Left && !rightDown)
        {
            if (owner.fSelectionRect.GetOperation() != SelectionRect::Operation::NoOp)
                owner.CancelSelection();
            else
                owner.ToggleFullScreen(LWS::Platform::isKeyPressed(LWS::KeyCode::Alt));
        }
        else if (event.button == MouseButton::Right && !leftDown)
        {
            owner.ExecutePredefinedCommand("PasteImageFromClipboard");
        }
    }

    int ViewerApplication::RawInputState::GetNavigationDirection() const
    {
        using namespace LInput;
        if (mouseDevicesState.empty())
            return 0;

        const auto& state = mouseDevicesState.begin()->second;
        if (state.GetButtonState(MouseButton::Forward) == ButtonState::Down)
            return 1;
        if (state.GetButtonState(MouseButton::Back) == ButtonState::Down)
            return -1;
        return 0;
    }

    void ViewerApplication::InitializeRawInput()
    {
        using namespace LInput;
        fRawInputState->rawInput.AddDevice(RawInput::UsagePage::GenericDesktopControls,
                                           RawInput::GenericDesktopControlsUsagePage::Mouse,
                                           RawInput::Flags::EnableBackground);
        fRawInputState->rawInput.OnInput.Add(
            [this](const RawInput::RawInputEvent& event)
            {
                HandleEventCallback(
                    [&]()
                    {
                        fRawInputState->OnRawInput(event);
                        return true;
                    });
            });
        fRawInputState->rawInput.Enable(true);
        fRawInputState->mouseClickEventHandler.OnMouseClickEvent.Add(
            [this](const MouseMultiClickHandler::EventArgs& event)
            {
                HandleEventCallback(
                    [&]()
                    {
                        fRawInputState->OnMouseMultiClick(event);
                        return true;
                    });
            });
    }

    int ViewerApplication::GetRawNavigationDirection() const
    {
        return fRawInputState->GetNavigationDirection();
    }

    void ViewerApplication::AddPlatformKeyBindings()
    {
        for (const auto& keyBinding : ConfigurationLoader::LoadKeyBindings())
        {
            fRawInputState->keyBindings.AddBinding(LInput::KeyCombination::FromString(keyBinding.KeyCombinationName),
                                                   {keyBinding.GroupID, std::string(), std::string()});
        }
    }

    bool ViewerApplication::handleKeyInput(const LWS::AnyEvent& eventData)
    {
        const auto* message = GetWinMessage(eventData);
        if (message == nullptr)
            return false;

        const LInput::KeyCombination keyCombination = LInput::KeyCombination::FromVirtualKey(
            static_cast<uint32_t>(message->wParam), static_cast<uint32_t>(message->lParam));
        LInput::KeyBindings<RawInputState::BindingElement>::ConcreteBindingType bindings;
        bool result = true;
        if (fRawInputState->keyBindings.GetBinding(keyCombination, bindings))
        {
            for (const auto& binding : bindings)
                result |= ExecutePredefinedCommand(binding.commandDescription);
        }
        return result;
    }

    std::intptr_t ViewerApplication::ClientWindwMessage(const LWS::AnyEvent& eventData)
    {
        const auto* message = GetWinMessage(eventData);
        if (message != nullptr && message->message == WM_SIZE)
        {
            fRefreshOperation.Begin();
            UpdateWindowSize();
            fRefreshOperation.End();
        }
        return 0;
    }

    bool ViewerApplication::HandleWinMessageEvent(const LWS::AnyEvent& eventData)
    {
        const auto* message = GetWinMessage(eventData);
        if (message == nullptr)
            return false;

        bool handled = false;
        switch (message->message)
        {
            case WM_SHOWWINDOW:
                if (!fIsFirstFrameDisplayed && message->wParam == TRUE)
                {
                    PostMessage(reinterpret_cast<HWND>(fWindow.GetHandle()),
                                Win32::UserMessage::PRIVATE_WN_FIRST_FRAME_DISPLAYED, 0, 0);
                    fIsFirstFrameDisplayed = true;
                }
                break;
            case Win32::UserMessage::PRIVATE_WN_FIRST_FRAME_DISPLAYED:
                AfterFirstFrameDisplayed();
                break;
            case Win32::UserMessage::PRIVATE_WN_AUTO_SCROLL:
                fAutoScroll->PerformAutoScroll();
                break;
            case Win32::UserMessage::PRIVATE_WM_NOTIFY_FILE_CHANGED:
                OnFileChangedImpl(reinterpret_cast<IFileWatcher::FileChangedEventArgs*>(message->wParam));
                break;
            case WM_COPYDATA:
            {
                const auto* copyData = reinterpret_cast<const COPYDATASTRUCT*>(message->lParam);
                if (message->wParam == Win32::UserMessage::PRIVATE_WM_LOAD_FILE_EXTERNALLY && copyData != nullptr)
                {
                    const auto* fileToLoad = reinterpret_cast<const wchar_t*>(copyData->lpData);
                    LoadFile(fileToLoad, IMCodec::PluginTraverseMode::NoTraverse);
                    fWindow.SetVisible(true);
                }
                break;
            }
            case WM_SYSKEYUP:
            case WM_KEYUP:
            {
                LInput::KeyCombination keyCombination = LInput::KeyCombination::FromVirtualKey(
                    static_cast<uint32_t>(message->wParam), static_cast<uint32_t>(message->lParam));
                const auto keyCode = keyCombination.keydata().keycode;
                if (keyCode == LInput::KeyCode::LALT || keyCode == LInput::KeyCode::RIGHTALT ||
                    keyCode == LInput::KeyCode::RALT)
                    fDoubleTap.SetState(false);
                break;
            }
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                handled = handleKeyInput(eventData);
                break;
            case WM_MOUSEMOVE:
                UpdateTexelPos();
                break;
            case WM_CLOSE:
                CloseApplication(false);
                break;
            case WM_ACTIVATE:
                SetAppActive(message->wParam != WA_INACTIVE);
                break;
            default:
                break;
        }
        return handled;
    }

    void ViewerApplication::CloseApplication(bool closeToTray)
    {
        const HANDLE mutex = CreateMutex(
            nullptr, FALSE, (LLUtils::native_string_type(Globals::ProgramGuid) + LLUTILS_TEXT("_CLOSEAPP")).c_str());
        if (mutex == nullptr)
            LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Mutex cannot be created.");

        const DWORD result = WaitForSingleObject(mutex, INFINITE);
        if (result != WAIT_OBJECT_0)
            LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Mutex ownership cannot be acquired.");

        fWindow.SetVisible(false);
        if (!closeToTray || FindTrayBarWindow() != 0)
            fWindow.Destroy();
        else
            fWindow.SetIsTrayWindow(true);

        if (!ReleaseMutex(mutex))
            LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Mutex cannot be released.");
        if (!CloseHandle(mutex))
            LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Mutex cannot be closed.");
    }

    bool ViewerApplication::HandleMessages(const LWS::AnyEvent& eventData)
    {
        if (GetWinMessage(eventData) != nullptr)
            return HandleWinMessageEvent(eventData);
        if (const auto* dragDropEvent = std::get_if<LWS::EventDragDropFile>(&eventData))
            return HandleFileDragDropEvent(*dragDropEvent);
        return false;
    }
}  // namespace OIV
