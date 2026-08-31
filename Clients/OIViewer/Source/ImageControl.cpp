#include "ImageControl.h"

#include <utility>

namespace OIV
{
    void ImageControl::InitializeEvents()
    {
        fImageList.Changed.Add(
            [this](ImageList::ChangeType change)
            {
                switch (change)
                {
                    case ImageList::ChangeType::ItemCount:
                        RefreshScrollInfo();
                        break;
                    case ImageList::ChangeType::ScrollPosition:
                        UpdateScrollPosition();
                        [[fallthrough]];
                    case ImageList::ChangeType::Visual:
                        RequestRepaint();
                        break;
                }
            });
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

    void ImageControl::SetImagePos(int pos)
    {
        fImageList.SetPos(pos);
    }

    ImageList& ImageControl::GetImageList()
    {
        return fImageList;
    }
}  // namespace OIV
