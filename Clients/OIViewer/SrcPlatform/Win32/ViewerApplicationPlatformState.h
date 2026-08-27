#pragma once

#include "MouseCaptureState.h"
#include "MouseMultiClickHandler.h"
#include "ViewerApplication.h"

#include <LInput/Buttons/ButtonStates.h>
#include <LInput/Buttons/Extensions/ButtonsStdExtension.h>
#include <LInput/Mouse/MouseButton.h>
#include <LInput/Keys/KeyBindings.h>
#include <LInput/Win32/RawInput/RawInput.h>

#include <LWS/NotificationIconGroup.hpp>

#include <Windows.h>

#include <map>

namespace OIV
{
    struct ViewerApplication::RawInputState
    {
        struct BindingElement
        {
            std::string commandDescription;
            std::string command;
            std::string arguments;
        };

        using MouseButtonType  = LInput::MouseButton;
        using MouseButtonState = LInput::ButtonsState<MouseButtonType, 8>;
        using MouseGroup       = std::map<uint8_t, MouseButtonState>;

        explicit RawInputState(ViewerApplication& application) : owner(application), mouseClickEventHandler(500, 2) {}

        void OnRawInput(const LInput::RawInput::RawInputEvent& event);
        void OnMouseEvent(const LInput::ButtonStdExtension<MouseButtonType>::ButtonEvent& event);
        void OnMouseInput(const LInput::RawInput::RawInputEventMouse& event);
        void OnMouseMultiClick(const MouseMultiClickHandler::EventArgs& event);
        [[nodiscard]] int GetNavigationDirection() const;

        ViewerApplication& owner;
        MouseGroup mouseDevicesState;
        LInput::RawInput rawInput;
        MouseCaptureState mouseCaptureState;
        MouseMultiClickHandler mouseClickEventHandler;
        LInput::KeyBindings<BindingElement> keyBindings;
    };

    struct ViewerApplication::NativeWindowState
    {
        LWS::NotificationIconGroup notificationIcons;
        HMODULE settingsModule                    = nullptr;
        DLL_DIRECTORY_COOKIE settingsDllDirectory = nullptr;
    };
}  // namespace OIV
