#pragma once

#include "ImageList.h"

#include <LWS/Window.hpp>

#include <cstdint>

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

        bool HandleWindowEvent(const LWS::AnyEvent& eventData);

        ImageList fImageList;
    };
}  // namespace OIV
