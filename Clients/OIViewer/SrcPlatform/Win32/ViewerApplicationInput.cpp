#include "ViewerApplication.h"

#include "Globals.h"
#include "UserMessages.h"
#include "ViewerApplicationPlatformState.h"
#include "ViewerMouseInput.h"

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

    void ViewerApplication::RawInputState::OnRawInput(const LInput::RawInput::RawInputEvent& event)
    {
        using namespace LInput;
        if (event.deviceType != RawInput::RawInputDeviceType::Mouse)
            return;

        const auto& mouse = static_cast<const RawInput::RawInputEventMouse&>(event);
        constexpr std::array buttons{LWS::MouseButton::Left, LWS::MouseButton::Right, LWS::MouseButton::Middle,
                                     LWS::MouseButton::X1, LWS::MouseButton::X2};
        auto& canvas           = owner.fWindow.GetCanvasWindow();
        const bool mouseInside = canvas.IsMouseInClientRect();
        for (size_t index = 0; index < buttons.size(); ++index)
        {
            if (mouse.buttonState[index] != ButtonState::NotSet)
                owner.fMouseInput->SetButton(event.deviceIndex, buttons[index],
                                             mouse.buttonState[index] == ButtonState::Down, mouseInside);
        }
        owner.fMouseInput->Move(event.deviceIndex, {mouse.deltaX, mouse.deltaY});
        if (mouse.wheelDelta != 0)
            owner.fMouseInput->Wheel(mouse.wheelDelta);
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
    }

    int ViewerApplication::GetRawNavigationDirection() const
    {
        return fMouseInput->GetNavigationDirection();
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
            {
                const bool active = LOWORD(message->wParam) != WA_INACTIVE;
                if (!active)
                    fMouseInput->Cancel();
                SetAppActive(active);
                break;
            }
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
