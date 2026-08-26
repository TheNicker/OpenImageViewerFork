#include <iomanip>
#include <filesystem>
#include <thread>
#include <future>
#include <cassert>

#include "ViewerApplication.h"

#include <Windows.h>
#include <Version.h>

#include <Functions.h>
#include <ApiGlobal.h>
#include <LWS/Platform.hpp>

#include <LInput/Keys/KeyCombination.h>
#include <LInput/Keys/KeyBindings.h>
#include <LInput/Buttons/Extensions/ButtonsStdExtension.h>
#include <LInput/Mouse/MouseButton.h>

#include <LLUtils/Exception.h>
#include <LLUtils/FileHelper.h>
#include <LLUtils/PlatformUtility.h>
#include <LLUtils/StringUtility.h>
#include <LLUtils/UniqueIDProvider.h>
#include <LLUtils/Logging/LogPredefined.h>
#include <LLUtils/Logging/Logger.h>
#include <LLUtils/FileSystemHelper.h>
#include <LLUtils/Rect.h>

#include <OIVAppCore/OIVHelper.h>
#include "Helpers/ClipboardSetup.h"
#include <OIVAppCore/MessageHelper.h>
#include <OIVAppCore/ShellIntegrationHelper.h>
#include "Helpers/ShellCommandHandler.h"

#include "Win32/UserMessages.h"
#include "OIVCommands.h"

#include "OIVImage/OIVFileImage.h"
#include "OIVImage/OIVRawImage.h"
#include "VirtualStatusBar.h"
#include "MonitorProvider.h"

#include "ContextMenu.h"
#include "Globals.h"
#include <OIVAppCore/ConfigurationLoader.h>
#include "CommandRegistry.h"
#include "ExceptionHandler.h"
#include <OIVAppCore/ColorCountPolicy.h>
#include <OIVAppCore/ColorCorrectionCommandPolicy.h>
#include <OIVAppCore/FileChangePolicy.h>
#include <OIVAppCore/FrameLimiterPolicy.h>
#include <OIVAppCore/ImageEditPolicy.h>
#include <OIVAppCore/ImageFormatCatalogPolicy.h>
#include <OIVAppCore/ImageLoadPresentationPolicy.h>
#include <OIVAppCore/ImageTransformCommandPolicy.h>
#include <OIVAppCore/InputGesturePolicy.h>
#include <OIVAppCore/OIVImageHelper.h>
#include <OIVAppCore/SelectionWorkflowPolicy.h>
#include <OIVAppCore/SequencerPolicy.h>
#include <OIVAppCore/SortCommandPolicy.h>
#include <OIVAppCore/SubImagePolicy.h>
#include <OIVAppCore/ViewActionController.h>
#include <OIVAppCore/ViewCommandPolicy.h>
#include <OIVAppCore/ViewerPresentationPolicy.h>
#include <OIVShared/PixelHelper.h>
#include <ImageUtil/ImageUtil.h>
#include "InterThreadMessages.h"

#include "Resource.h"

namespace OIV
{
    void ViewerApplication::OnMouseEvent(const LInput::ButtonStdExtension<MouseButtonType>::ButtonEvent& btnEvent)
    {
        using namespace LInput;
        bool isMouseCursorOnTopOfWindowAndInsideClientRect = fWindow.IsUnderMouseCursor() &&
                                                             fWindow.IsMouseCursorInClientRect();
        if (btnEvent.button == MouseButton::Middle && btnEvent.eventType == EventType::Pressed &&
            isMouseCursorOnTopOfWindowAndInsideClientRect)
        {
            fAutoScroll->ToggleAutoScroll();
            if (fAutoScroll->IsAutoScrolling() == false)
            {
                fWindow.SetCursorType(::OIV::Win32::MainWindow::CursorType::SystemDefault);
                fAutoScrollAnchor.reset();
            }
            else
            {
                LLUtils::native_string_type anchorPath  = LLUtils::StringUtility::ToNativeString(
                                                              LLUtils::PlatformUtility::GetExeFolder()) +
                                                          LLUTILS_TEXT("./Resources/Cursors/ArrowC.cur");
                std::unique_ptr<OIVFileImage> fileImage = std::make_unique<OIVFileImage>(anchorPath);
                if (fileImage->Load(&fImageLoader, IMCodec::PluginTraverseMode::AnyPlugin) == RC_Success)
                {
                    fileImage->SetImageRenderMode(OIV_Image_Render_mode::IRM_Overlay);
                    fileImage->SetPosition(static_cast<LLUtils::PointF64>(
                        static_cast<LLUtils::PointI32>(fWindow.GetMousePosition()) -
                        static_cast<LLUtils::PointI32>(fileImage->GetImage()->GetDimensions()) / 2));
                    fileImage->SetScale({1.0, 1.0});
                    fileImage->SetOpacity(0.5);
                    fileImage->SetVisible(true);
                    fAutoScrollAnchor = std::move(fileImage);
                    // TODO: do we need update here when loading the cursor anchor ?
                }
            }
        }

        if (btnEvent.button == MouseButton::Left && btnEvent.eventType == EventType::Released)
        {
            fWindow.SetLockMouseToWindowMode(LWS::LockMouseToWindowMode::NoLock);
        }

        LWS::LockMouseToWindowMode lockMode = LWS::LockMouseToWindowMode::NoLock;
        const auto& mouseState              = fMouseDevicesState.find(btnEvent.parent->GetID())->second;
        const bool IsRightDown              = mouseState.GetButtonState(MouseButtonType::Right) == ButtonState::Down;
        const bool IsLeftDown               = mouseState.GetButtonState(MouseButtonType::Left) == ButtonState::Down;
        const bool IsRightCatured           = fMouseCaptureState.IsCaptured(MouseButtonType::Right);

        if (btnEvent.button == MouseButton::Left)
        {
            if (btnEvent.eventType == EventType::Pressed && IsRightDown &&
                isMouseCursorOnTopOfWindowAndInsideClientRect)
            {
                // Rocker gesture - navigate backward
                fRockerGestureActivate = true;
                fContextMenuTimer.SetInterval(0);
                JumpFiles(-1);
            }
            else if (IsRightDown == false && IsRightCatured == false)
            {
                // Window drag and resize
                if (LWS::Platform::isKeyPressed(LWS::KeyCode::Alt) == false && fWindow.IsFullScreen() == false)
                {
                    if (LWS::Platform::isKeyPressed(LWS::KeyCode::Control) == true)
                        lockMode = LWS::LockMouseToWindowMode::LockResize;
                    else
                        lockMode = LWS::LockMouseToWindowMode::LockMove;
                }
            }
            if (btnEvent.eventType == EventType::Released)
            {
                lockMode = LWS::LockMouseToWindowMode::NoLock;
            }

            fWindow.SetLockMouseToWindowMode(lockMode);

            if (LWS::Platform::isKeyPressed(LWS::KeyCode::Alt))
            {
                SelectionRect::Operation op = SelectionRect::Operation::NoOp;
                if (btnEvent.eventType == EventType::Pressed && fWindow.IsUnderMouseCursor())
                    op = SelectionRect::Operation::BeginDrag;
                else if (btnEvent.eventType == EventType::Released && fWindow.IsUnderMouseCursor())
                    op = SelectionRect::Operation::EndDrag;

                fSelectionRect.SetSelection(op, SnapToScreenSpaceImagePixels(fWindow.GetMousePosition()));
                SaveImageSpaceSelection();
            }
        }
        if (btnEvent.button == MouseButton::Back || btnEvent.button == MouseButton::Forward)
        {
            if (btnEvent.eventType == EventType::Pressed && fWindow.IsUnderMouseCursor())
            {
                fTimerNavigation.SetInterval(fQuickBrowseDelay);
            }
            else
            {
                if (fMouseCaptureState.IsCaptured(MouseButton::Back) == false &&
                    fMouseCaptureState.IsCaptured(MouseButton::Forward) == false)
                    fTimerNavigation.SetInterval(0);
            }
        }

        if (btnEvent.button == MouseButton::Right && btnEvent.eventType == EventType::Pressed &&
            isMouseCursorOnTopOfWindowAndInsideClientRect)
        {
            // Rocker gesture - navigate forward
            if (IsLeftDown)
            {
                fRockerGestureActivate = true;
                fContextMenuTimer.SetInterval(0);
                JumpFiles(1);
            }

            if (fContextMenuTimer.GetInterval() == 0 && fRockerGestureActivate == false)
            {
                fContextMenuTimer.SetInterval(500);
                fDownPosition = LWS::Platform::getMousePosition();
            }
        }
    }

    void ViewerApplication::OnMouseInput(const LInput::RawInput::RawInputEventMouse& mouseInput)
    {
        using namespace LInput;
        const auto& mouseState = fMouseDevicesState.find(mouseInput.deviceIndex)->second;

        // const bool IsLeftDown = mouseState.GetButtonState(MouseState::Button::Left) == MouseState::State::Down;
        const bool IsLeftDown  = mouseState.GetButtonState(MouseButtonType::Left) == ButtonState::Down;
        const bool IsRightDown = mouseState.GetButtonState(MouseButtonType::Right) == ButtonState::Down;

        const bool IsRightCatured = fMouseCaptureState.IsCaptured(MouseButtonType::Right);
        const bool IsLeftCaptured = fMouseCaptureState.IsCaptured(MouseButtonType::Left);
        // const bool IsRightDown = mouseState.GetButtonState(MouseState::Button::Right) == MouseState::State::Down;
        // const bool IsLeftReleased = evnt->GetButtonEvent(MouseState::Button::Left) == MouseState::EventType::Released;

        const bool isMouseUnderCursor = fWindow.IsUnderMouseCursor();

        // Quick browse feature
        // const bool isNavigationBackwardDown = (mouseState.GetButtonState(MouseButtonType::Back) ==
        // ButtonState::Down); const bool isNavigationBackwardUp = (mouseState.GetButtonState(MouseButtonType::Back) ==
        // ButtonState::Up); const bool isNavigationBackwardUp = (mouseState.GetButtonState(MouseState::Button::Third)
        // == MouseState::State::Up); const bool isNavigationForwardDown =
        // mouseState.GetButtonState(MouseButtonType::Forward) == ButtonState::Down; const bool isNavigationForwardUp =
        // mouseState.GetButtonState(MouseButtonType::Forward) == ButtonState::Up;

        // Selection rect
        if (LWS::Platform::isKeyPressed(LWS::KeyCode::Alt))
        {
            if (IsLeftCaptured)
            {
                auto snappedPOsition = SnapToScreenSpaceImagePixels(fWindow.GetMousePosition());
                fSelectionRect.SetSelection(SelectionRect::Operation::Drag, snappedPOsition);
                SaveImageSpaceSelection();
            }
        }

        if (IsRightCatured == true && fContextMenu->IsVisible() == false)
        {
            if (mouseInput.deltaX != 0 || mouseInput.deltaY != 0)
                Pan(LLUtils::PointF64(mouseInput.deltaX, mouseInput.deltaY));
        }

        LONG wheelDelta = mouseInput.wheelDelta;

        if (wheelDelta != 0)
        {
            // Browse files
            if (isMouseUnderCursor && LWS::Platform::isKeyPressed(LWS::KeyCode::Alt))
            {
                ExecutePredefinedCommand(wheelDelta > 0 ? "PreviousSubImage" : "NextSubImage");
            }
            else if (isMouseUnderCursor && LWS::Platform::isKeyPressed(LWS::KeyCode::Shift))
            {
                ExecutePredefinedCommand(wheelDelta > 0 ? "PreviousImageInFolder" : "NextImageInFolder");
            }
            else if (IsRightCatured || isMouseUnderCursor)
            {
                auto mousePos = fWindow.GetMousePosition();
                // 20% percent zoom in each wheel step
                if (IsRightCatured)
                    //  Zoom to center of the client area if currently panning.
                    Zoom(wheelDelta * 0.2);
                else
                    Zoom(wheelDelta * 0.2, mousePos.x, mousePos.y);
            }
        }

        if (IsRightDown)
        {
            LLUtils::PointI32 currentPosition = LWS::Platform::getMousePosition();
            if (currentPosition.DistanceSquared(fDownPosition) > 25)
                fContextMenuTimer.SetInterval(0);
        }
        else
        {
            fContextMenuTimer.SetInterval(0);
        }

        if (IsLeftDown == false && IsRightDown == false)
            fRockerGestureActivate = false;
    }

    void ViewerApplication::OnRawInput(const LInput::RawInput::RawInputEvent& evnt)
    {
        using namespace LInput;
        if (evnt.deviceType == RawInput::RawInputDeviceType::Mouse)
        {
            const auto& mouseEvent = static_cast<const RawInput::RawInputEventMouse&>(evnt);

            // Add button states for multiple mouses.
            auto it = fMouseDevicesState.find(evnt.deviceIndex);
            // if mouse ID not found add new buttonstates entry.
            if (it == std::end(fMouseDevicesState))
            {
                it = fMouseDevicesState.emplace(evnt.deviceIndex, decltype(fMouseDevicesState)::mapped_type()).first;
                // Add standard extension
                auto stdExtension = std::make_shared<ButtonStdExtension<MouseButtonType>>(evnt.deviceIndex, 250, 0);
                stdExtension->OnButtonEvent.Add(
                    std::bind(&ViewerApplication::OnMouseEvent, this, std::placeholders::_1));
                it->second.AddExtension(std::static_pointer_cast<IButtonStateExtension<MouseButtonType>>(stdExtension));

                // Add multitap extension for click, double click and triple click
                /*
                auto multitapextension = std::make_shared<MultitapExtension<MouseButtonType>>(evnt.deviceIndex, 500, 2);
                multitapextension->OnButtonEvent.Add(std::bind(&ViewerApplication::OnMouseMultiTap,
                this,std::placeholders::_1));
                it->second.AddExtension(std::static_pointer_cast<IButtonStateExtension<MouseButtonType>>(multitapextension));
                */
            }

            for (size_t i = 0; i < RawInput::MaxMouseButtons; i++)
            {
                it->second.SetButtonState(
                    static_cast<decltype(fMouseDevicesState)::mapped_type::underlying_button_type>(i),
                    mouseEvent.buttonState[i]);

                const bool mouseUnderWindow = mouseEvent.buttonState[i] == ButtonState::Down &&
                                              fWindow.IsUnderMouseCursor();
                fMouseCaptureState.Update(static_cast<MouseButton>(i), mouseEvent.buttonState[i], mouseUnderWindow);

                fMouseClickEventHandler.SetButtonState(static_cast<MouseButton>(i), mouseEvent.buttonState[i]);
            }

            fMouseClickEventHandler.SetMouseDelta(mouseEvent.deltaX, mouseEvent.deltaY);

            OnMouseInput(mouseEvent);
        }
    }

    void ViewerApplication::OnMouseMultiClick(const MouseMultiClickHandler::EventArgs& args)
    {
        using namespace LInput;
        if (args.clickCount == 2 && fWindow.IsMouseCursorInClientRect() && fWindow.IsUnderMouseCursor())
        {
            const auto& mouseState = fMouseDevicesState.begin()->second;
            const bool IsRightDown = mouseState.GetButtonState(MouseButtonType::Right) == ButtonState::Down;
            const bool IsLeftDown  = mouseState.GetButtonState(MouseButtonType::Left) == ButtonState::Down;

            if (args.button == MouseButton::Left)
            {
                if (IsRightDown == false)
                {
                    if (fSelectionRect.GetOperation() != SelectionRect::Operation::NoOp)
                    {
                        CancelSelection();
                    }
                    else
                    {
                        ToggleFullScreen(LWS::Platform::isKeyPressed(LWS::KeyCode::Alt) ? true : false);
                    }
                }
            }

            if (args.button == MouseButton::Right)
            {
                if (IsLeftDown == false)
                {
                    ExecutePredefinedCommand("PasteImageFromClipboard");
                }
            }
        }
    }

    bool ViewerApplication::handleKeyInput(const LWS::Win32::WinMessage& message)
    {
        LInput::KeyCombination keyCombination = LInput::KeyCombination::FromVirtualKey(
            static_cast<uint32_t>(message.wParam), static_cast<uint32_t>(message.lParam));
        LInput::KeyBindings<BindingElement>::ConcreteBindingType bindings;
        bool result = true;
        if (result == fKeyBindings.GetBinding(keyCombination, bindings))
        {
            for (const auto& binding : bindings)
                result |= ExecutePredefinedCommand(binding.commandDescription);
        }

        return result;
    }

    LRESULT ViewerApplication::ClientWindwMessage(const LWS::AnyEvent& eventData)
    {
        const auto* raw = std::get_if<LWS::EventRawPlatform>(&eventData);
        if (raw == nullptr || raw->platformType != std::to_underlying(LWS::BackendId::Win32) ||
            raw->platformData == nullptr)
            return 0;

        const LWS::Win32::WinMessage& message = *reinterpret_cast<const LWS::Win32::WinMessage*>(raw->platformData);

        LRESULT retValue = 0;
        switch (message.message)
        {
            case WM_SIZE:
                fRefreshOperation.Begin();
                UpdateWindowSize();
                fRefreshOperation.End();
                break;
        }
        return retValue;
    }

    void ViewerApplication::SetTopMostUserMesage()
    {
        SetUserMessage(ViewerPresentationPolicy::FormatTopMostMessage(fTopMostCounter),
                       static_cast<GroupID>(UserMessageGroups::WindowOnTop),
                       MessageFlags::Interchangeable | MessageFlags::ManualRemove);
    }

    bool ViewerApplication::GetAppActive() const
    {
        return fIsActive;
    }

    void ViewerApplication::SetAppActive(bool active)
    {
        if (active != fIsActive)
        {
            fIsActive = active;
            if (fIsActive == true && fFileReloadPolicy.HasPendingReloadFor(GetOpenedFileName()))
            {
                PerformReloadFile(GetOpenedFileName());
            }
            else
            {
                UpdateTitle();
            }
        }
    }

    void ViewerApplication::ProcessTopMost()
    {
        if (fTopMostCounter > 0)
        {
            fTopMostCounter--;

            if (fTopMostCounter == 0)
            {
                fTimerTopMostRetention.SetInterval(0);
                fWindow.SetAlwaysOnTop(false);
                fMessageManager->RemoveGroup(static_cast<GroupID>(UserMessageGroups::WindowOnTop));
            }
            else
                SetTopMostUserMesage();
        }
    }

    bool ViewerApplication::HandleWinMessageEvent(const LWS::Win32::WinMessage& uMsg)
    {
        bool handled = false;

        switch (uMsg.message)
        {
            case WM_SHOWWINDOW:
                if (fIsFirstFrameDisplayed == false && uMsg.wParam == TRUE)
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
                OnFileChangedImpl(reinterpret_cast<IFileWatcher::FileChangedEventArgs*>(uMsg.wParam));
                break;

            case WM_COPYDATA:
            {
                COPYDATASTRUCT* cds = (COPYDATASTRUCT*) uMsg.lParam;
                if (uMsg.wParam == ::OIV::Win32::UserMessage::PRIVATE_WM_LOAD_FILE_EXTERNALLY)
                {
                    wchar_t* fileToLoad = reinterpret_cast<wchar_t*>(cds->lpData);
                    LoadFile(fileToLoad, IMCodec::PluginTraverseMode::NoTraverse);
                    fWindow.SetVisible(true);
                }
            }
            break;

            case WM_SYSKEYUP:
            case WM_KEYUP:
            {
                using namespace LInput;
                KeyCombination keyCombination = KeyCombination::FromVirtualKey(static_cast<uint32_t>(uMsg.wParam),
                                                                               static_cast<uint32_t>(uMsg.lParam));

                bool isAltup = (keyCombination.keydata().keycode == KeyCode::LALT ||
                                keyCombination.keydata().keycode == KeyCode::RIGHTALT ||
                                keyCombination.keydata().keycode == KeyCode::RALT);

                if (isAltup)
                    fDoubleTap.SetState(false);
            }
            break;
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                handled = handleKeyInput(uMsg);
                break;

            case WM_MOUSEMOVE:
                UpdateTexelPos();
                break;
            case WM_CLOSE:
                CloseApplication(false);
                break;
            case WM_ACTIVATE:
                SetAppActive(uMsg.wParam != WA_INACTIVE);
                break;
        }

        return handled;
    }

    void ViewerApplication::CloseApplication(bool closeToTray)
    {
        HANDLE mutex = CreateMutex(
            NULL, FALSE, (LLUtils::native_string_type(Globals::ProgramGuid) + LLUTILS_TEXT("_CLOSEAPP")).c_str());
        if (mutex == nullptr)
        {
            LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Mutex cannot be created.");
        }

        const DWORD result = WaitForSingleObject(mutex,      // handle to mutex
                                                 INFINITE);  // no time-out interval

        switch (result)
        {
            case WAIT_OBJECT_0:

                fWindow.SetVisible(false);

                if (closeToTray == false || FindTrayBarWindow() != nullptr)
                {
                    fWindow.Destroy();
                }
                else
                {
                    fWindow.SetIsTrayWindow(true);
                }

                break;
            default:
                LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Mutex ownership cannot be acquired.");
        }

        if (!ReleaseMutex(mutex))
            LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Mutex cannot be released.");

        if (CloseHandle(mutex) == FALSE)
            LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Mutex cannot be closed.");
    }

    bool ViewerApplication::HandleFileDragDropEvent(const LWS::EventDragDropFile& eventDragDropFile)
    {
        LLUtils::native_string_type normalizedPath =
            std::filesystem::path(eventDragDropFile.fileName).lexically_normal().wstring();
        if (LoadFileOrFolder(normalizedPath,
                             IMCodec::PluginTraverseMode::AnyPlugin | IMCodec::PluginTraverseMode::AnyFileType))
        {
            fWindow.SetForground();
            return true;
        }

        return false;
    }

    bool ViewerApplication::HandleClientWindowMessages(const LWS::AnyEvent& eventData)
    {
        return ClientWindwMessage(eventData) != 0;
    }

    bool ViewerApplication::HandleMessages(const LWS::AnyEvent& eventData)
    {
        if (const auto* raw = std::get_if<LWS::EventRawPlatform>(&eventData);
            raw != nullptr && raw->platformType == std::to_underlying(LWS::BackendId::Win32) &&
            raw->platformData != nullptr)
        {
            return HandleWinMessageEvent(*reinterpret_cast<const LWS::Win32::WinMessage*>(raw->platformData));
        }

        if (const auto* dragDropEvent = std::get_if<LWS::EventDragDropFile>(&eventData))
            return HandleFileDragDropEvent(*dragDropEvent);

        return false;
    }
}  // namespace OIV
