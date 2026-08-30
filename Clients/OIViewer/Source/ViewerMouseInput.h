#pragma once

#include "MouseCaptureState.h"
#include "MouseMultiClickHandler.h"

#include <LWS/MouseButton.hpp>
#include <LWS/Event.hpp>

#include <array>
#include <cstdint>
#include <map>

namespace OIV
{
    class ViewerApplication;

    class ViewerMouseInput final
    {
      public:

        explicit ViewerMouseInput(ViewerApplication& owner);

        void SetButton(uint8_t deviceId, LWS::MouseButton button, bool pressed, bool mouseInside);
        void Move(uint8_t deviceId, LWS::Point delta);
        void Wheel(double steps);
        void Cancel();
        [[nodiscard]] int GetNavigationDirection() const;

      private:

        static constexpr size_t ButtonCount = static_cast<size_t>(LWS::MouseButton::Count);
        using ButtonState                   = std::array<bool, ButtonCount>;

        void OnButton(uint8_t deviceId, LWS::MouseButton button, bool pressed, bool mouseInside);
        void OnMultiClick(const MouseMultiClickHandler::EventArgs& event);
        [[nodiscard]] const ButtonState* FindDevice(uint8_t deviceId) const;

        ViewerApplication& fOwner;
        std::map<uint8_t, ButtonState> fDevices;
        MouseCaptureState fCapture;
        MouseMultiClickHandler fMultiClick{500, 2};
        LLUtils::Point<int64_t> fRightDragDelta{};
    };
}  // namespace OIV
