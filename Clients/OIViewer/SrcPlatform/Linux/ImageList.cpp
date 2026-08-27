#include "ImageList.h"

#include <LLUtils/Exception.h>

struct ImageList::NativeState
{
    LWS::Handle targetWindow = 0;
};

ImageList::ImageList() : fNativeState(std::make_unique<NativeState>()) {}
ImageList::~ImageList() = default;

void ImageList::SetTarget(LWS::Handle windowHandle)
{
    fNativeState->targetWindow = windowHandle;
}

std::optional<int32_t> ImageList::GetViewportHeight() const
{
    return std::nullopt;
}

void ImageList::PrepareImage([[maybe_unused]] ImageDesc& imageDesc) {}
void ImageList::RequestRepaint([[maybe_unused]] bool erase) {}

size_t ImageList::GetNumberOfDisplayedElements()
{
    LL_EXCEPTION_NOT_IMPLEMENT("Native subimage viewport queries are not implemented on Linux");
}

void ImageList::Draw()
{
    LL_EXCEPTION_NOT_IMPLEMENT("Native subimage rendering is not implemented on Linux");
}
