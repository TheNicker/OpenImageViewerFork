#include "ViewerApplication.h"

#include "Globals.h"
#include "CopyDataProtocol.h"
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
        std::ignore = LWS::Win32::SetPlatformCallback(fWindow,
                                                      [this](const LWS::Win32::PlatformEvent& event)
                                                      {
                                                          std::optional<LRESULT> result;
                                                          HandleEventCallback(
                                                              [&]()
                                                              {
                                                                  result = fRawInputState->HandlePlatformEvent(event);
                                                                  return false;
                                                              });
                                                          return result;
                                                      });
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

    void ViewerApplication::RawInputState::HandleKeyInput(const LWS::Win32::KeyEvent& eventData)
    {
        if (!eventData.pressed)
            return;

        const LInput::KeyCombination keyCombination = LInput::KeyCombination::FromVirtualKey(eventData.virtualKey,
                                                                                             eventData.keyData);
        LInput::KeyBindings<RawInputState::BindingElement>::ConcreteBindingType bindings;
        if (keyBindings.GetBinding(keyCombination, bindings))
        {
            for (const auto& binding : bindings)
                owner.ExecutePredefinedCommand(binding.commandDescription);
        }
    }

    std::intptr_t ViewerApplication::ClientWindwMessage(const LWS::AnyEvent& eventData)
    {
        if (std::holds_alternative<LWS::EventResize>(eventData))
        {
            fRefreshOperation.Begin();
            UpdateWindowSize();
            fRefreshOperation.End();
        }
        return 0;
    }

    std::optional<LRESULT> ViewerApplication::RawInputState::HandlePlatformEvent(
        const LWS::Win32::PlatformEvent& eventData)
    {
        if (const auto* key = std::get_if<LWS::Win32::KeyEvent>(&eventData))
        {
            if (!key->pressed)
            {
                const auto keyCode =
                    LInput::KeyCombination::FromVirtualKey(key->virtualKey, key->keyData).keydata().keycode;
                if (keyCode == LInput::KeyCode::LALT || keyCode == LInput::KeyCode::RIGHTALT ||
                    keyCode == LInput::KeyCode::RALT)
                    owner.fDoubleTap.SetState(false);
            }
            HandleKeyInput(*key);
            return std::nullopt;
        }
        if (const auto* activation = std::get_if<LWS::Win32::ActivationEvent>(&eventData))
        {
            if (!activation->active)
                owner.fMouseInput->Cancel();
            else
                owner.fWindow.SetIsTrayWindow(false);
            owner.SetAppActive(activation->active);
            return std::nullopt;
        }
        if (const auto* copyData = std::get_if<LWS::Win32::CopyDataEvent>(&eventData);
            copyData != nullptr && copyData->identifier == Win32::LoadFileCopyDataId &&
            copyData->data.size() >= sizeof(wchar_t) && copyData->data.size() % sizeof(wchar_t) == 0)
        {
            const auto* fileToLoad      = reinterpret_cast<const wchar_t*>(copyData->data.data());
            const size_t characterCount = copyData->data.size() / sizeof(wchar_t);
            if (fileToLoad[characterCount - 1] == L'\0')
            {
                owner.LoadFile(fileToLoad, IMCodec::PluginTraverseMode::NoTraverse);
                owner.fWindow.SetVisible(true);
                return TRUE;
            }
        }
        return std::nullopt;
    }

    bool ViewerApplication::HandleWinMessageEvent(const LWS::AnyEvent& eventData)
    {
        if (std::holds_alternative<LWS::EventMouseMove>(eventData))
            UpdateTexelPos();
        else if (std::holds_alternative<LWS::EventClose>(eventData))
            CloseApplication(false);
        else if (std::holds_alternative<LWS::EventPaint>(eventData) && !fIsFirstFrameDisplayed)
        {
            fIsFirstFrameDisplayed = true;
            AfterFirstFrameDisplayed();
        }
        return false;
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
        if (const auto* dragDropEvent = std::get_if<LWS::EventDragDropFile>(&eventData))
            return HandleFileDragDropEvent(*dragDropEvent);
        return HandleWinMessageEvent(eventData);
    }
}  // namespace OIV
