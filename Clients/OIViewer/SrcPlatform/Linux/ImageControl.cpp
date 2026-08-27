#include "ImageControl.h"

#include <LLUtils/Exception.h>

namespace OIV
{
    void ImageControl::RefreshScrollInfo()
    {
        LL_EXCEPTION_NOT_IMPLEMENT("Native subimage scrolling is not implemented on Linux");
    }

    std::intptr_t ImageControl::SendMessage([[maybe_unused]] std::uint32_t message,
                                            [[maybe_unused]] std::uintptr_t wParam,
                                            [[maybe_unused]] std::intptr_t lParam)
    {
        LL_EXCEPTION_NOT_IMPLEMENT("Native subimage messages are not implemented on Linux");
    }

    bool ImageControl::HandleWindowEvent(const LWS::AnyEvent& eventData)
    {
        if (std::holds_alternative<LWS::EventPaint>(eventData))
        {
            fImageList.Draw();
            return true;
        }
        return false;
    }
}  // namespace OIV
