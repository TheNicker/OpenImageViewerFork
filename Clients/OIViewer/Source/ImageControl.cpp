#include "ImageControl.h"

#include <utility>

namespace OIV
{
    ImageControl::ImageControl()
    {
        std::ignore = AddEventListener(
            [this](const LWS::AnyEvent& eventData) noexcept
            {
                try
                {
                    return HandleWindowEvent(eventData);
                }
                catch (...)
                {
                    return true;
                }
            });
    }

    ImageControl::~ImageControl() = default;

    void ImageControl::SetImagePos(int pos)
    {
        fImageList.SetPos(pos);
    }

    ImageList& ImageControl::GetImageList()
    {
        return fImageList;
    }
}  // namespace OIV
