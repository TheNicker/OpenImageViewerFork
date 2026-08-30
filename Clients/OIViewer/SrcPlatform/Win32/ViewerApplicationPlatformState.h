#pragma once

#include "ViewerApplication.h"

#include <LInput/Keys/KeyBindings.h>
#include <LInput/Win32/RawInput/RawInput.h>

#include <LWS/NotificationIconGroup.hpp>

#include <Windows.h>

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

        explicit RawInputState(ViewerApplication& application) : owner(application) {}

        void OnRawInput(const LInput::RawInput::RawInputEvent& event);

        ViewerApplication& owner;
        LInput::RawInput rawInput;
        LInput::KeyBindings<BindingElement> keyBindings;
    };

    struct ViewerApplication::NativeWindowState
    {
        LWS::NotificationIconGroup notificationIcons;
        HMODULE settingsModule                    = nullptr;
        DLL_DIRECTORY_COOKIE settingsDllDirectory = nullptr;
    };
}  // namespace OIV
