#include "ImageList.h"

#include <algorithm>

int ImageList::GetSelected() const
{
    return fSelected;
}

void ImageList::Clear()
{
    fImages.clear();
}

void ImageList::SetSelected(int selected)
{
    if (selected == fSelected)
        return;

    fSelected  = selected;
    bool erase = false;

    if (const std::optional<int32_t> viewportHeight = GetViewportHeight())
    {
        const int maxEntries = *viewportHeight / static_cast<int>(fEntryHeight);
        if (fSelected < fPos)
        {
            fPos = fSelected;
        }
        else if (maxEntries > 0)
        {
            const int posOffset = *viewportHeight % static_cast<int>(fEntryHeight) == 0 ? 0 : 1;
            if (fSelected - fPos > maxEntries - posOffset)
                fPos = std::min(static_cast<int>(GetNumberOfElements()), fSelected - maxEntries + posOffset);
            erase = true;
        }
    }

    const ImageSelectionChangeArgs args{.imageIndex = selected};
    ImageSelectionChanged.Raise(args);
    RequestRepaint(erase);
}

void ImageList::MouseClick([[maybe_unused]] int xPos, int yPos)
{
    const int selected = yPos / static_cast<int>(fEntryHeight) + fPos;
    if (selected < static_cast<int>(fImages.size()))
        SetSelected(selected);
}

void ImageList::SetPos(int pos)
{
    fPos = pos;
}

size_t ImageList::GetNumberOfElements() const
{
    return fImages.size();
}

void ImageList::SetImage(const ImageDesc& imageDesc)
{
    fImages.resize(imageDesc.index + 1);
    fImages[imageDesc.index] = imageDesc;
    PrepareImage(fImages[imageDesc.index]);
    RequestRepaint(true);
}
