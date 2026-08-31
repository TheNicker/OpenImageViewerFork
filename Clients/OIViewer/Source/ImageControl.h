#pragma once

#include "ImageList.h"

#include <LWS/Window.hpp>

#include <cstdint>
#include <memory>

namespace OIV
{
    class ImageControl : public LWS::Window
    {
      public:

        ImageControl();
        ~ImageControl() override;

        void SetImagePos(int pos);
        ImageList& GetImageList();
        void RefreshScrollInfo();
        std::intptr_t SendMessage(std::uint32_t message, std::uintptr_t wParam, std::intptr_t lParam);

      private:

        struct NativeState;

        bool HandleWindowEvent(const LWS::AnyEvent& eventData);
        void InitializeEvents();
        void InitializePlatformRendering();
        void RequestRepaint();
        void UpdateScrollPosition();

        ImageList fImageList;
        std::unique_ptr<NativeState> fNativeState;
    };
}  // namespace OIV
