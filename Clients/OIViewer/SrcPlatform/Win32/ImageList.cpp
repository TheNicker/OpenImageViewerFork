#include "ImageList.h"

#include <Windows.h>

#include <algorithm>

struct ImageList::NativeState
{
    NativeState()
    {
        LOGFONT fontDescription{};
        fontDescription.lfHeight  = 20;
        fontDescription.lfWeight  = FW_NORMAL;
        fontDescription.lfQuality = CLEARTYPE_QUALITY;
        wcscpy_s(fontDescription.lfFaceName, LLUTILS_TEXT("Segoe UI"));
        font           = CreateFontIndirect(&fontDescription);
        grayBrush      = CreateSolidBrush(RGB(245, 249, 213));
        lightGrayBrush = CreateSolidBrush(RGB(224, 249, 213));
        blueBrush      = CreateSolidBrush(RGB(0, 0, 200));
        pen            = CreatePen(PS_SOLID, ImageList::fLineWidth, RGB(0, 0, 0));
        verticalPen    = CreatePen(PS_SOLID, 1, RGB(255, 0, 255));
        verticalPen2   = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
    }

    ~NativeState()
    {
        if (verticalPen2 != nullptr)
            DeleteObject(verticalPen2);
        if (verticalPen != nullptr)
            DeleteObject(verticalPen);
        if (pen != nullptr)
            DeleteObject(pen);
        if (grayBrush != nullptr)
            DeleteObject(grayBrush);
        if (lightGrayBrush != nullptr)
            DeleteObject(lightGrayBrush);
        if (blueBrush != nullptr)
            DeleteObject(blueBrush);
        if (font != nullptr)
            DeleteObject(font);
    }

    HFONT font            = nullptr;
    HBRUSH grayBrush      = nullptr;
    HBRUSH lightGrayBrush = nullptr;
    HBRUSH blueBrush      = nullptr;
    HPEN pen              = nullptr;
    HPEN verticalPen      = nullptr;
    HPEN verticalPen2     = nullptr;
    HWND targetWindow     = nullptr;
};

ImageList::ImageList() : fNativeState(std::make_unique<NativeState>()) {}
ImageList::~ImageList() = default;

void ImageList::SetTarget(LWS::Handle windowHandle)
{
    fNativeState->targetWindow = reinterpret_cast<HWND>(windowHandle);
}

std::optional<int32_t> ImageList::GetViewportHeight() const
{
    RECT rect{};
    if (fNativeState->targetWindow == nullptr || !GetClientRect(fNativeState->targetWindow, &rect))
        return std::nullopt;
    return rect.bottom - rect.top;
}

void ImageList::PrepareImage(ImageDesc& imageDesc)
{
    if (imageDesc.bitmap != nullptr)
        imageDesc.bitmap = imageDesc.bitmap->resize(64, 64, 255);
    if (imageDesc.mask != nullptr)
        imageDesc.mask = imageDesc.mask->resize(64, 64, 0);
}

size_t ImageList::GetNumberOfDisplayedElements()
{
    const std::optional<int32_t> viewportHeight = GetViewportHeight();
    if (!viewportHeight)
        return 0;
    const size_t maximumVisibleElements = static_cast<size_t>(*viewportHeight) / fEntryHeight;
    return std::min(maximumVisibleElements, GetNumberOfElements());
}

void ImageList::RequestRepaint(bool erase)
{
    if (fNativeState->targetWindow != nullptr)
        InvalidateRect(fNativeState->targetWindow, nullptr, erase ? TRUE : FALSE);
}

void ImageList::Draw()
{
    const HWND window = fNativeState->targetWindow;
    if (window == nullptr)
        return;

    RECT clientRect{};
    GetClientRect(window, &clientRect);
    constexpr int imageDestWidth  = 64;
    constexpr int imageDestHeight = 64;
    const int entryWidth          = clientRect.right - clientRect.left;
    constexpr int imagePosition   = 30;

    PAINTSTRUCT paint{};
    BeginPaint(window, &paint);
    HDC memoryDc = CreateCompatibleDC(nullptr);
    SelectObject(paint.hdc, fNativeState->pen);
    SelectObject(paint.hdc, fNativeState->font);

    int currentEntry = 0;
    int y            = fPos * -static_cast<int>(fEntryHeight);
    for (const ImageDesc& imageDesc : fImages)
    {
        RECT entryRect{0, y, entryWidth, y + static_cast<int>(fEntryHeight)};
        if (currentEntry == fSelected)
        {
            FillRect(paint.hdc, &entryRect, fNativeState->blueBrush);
            SetTextColor(paint.hdc, RGB(255, 255, 255));
        }
        else
        {
            FillRect(paint.hdc, &entryRect,
                     currentEntry % 2 == 0 ? fNativeState->grayBrush : fNativeState->lightGrayBrush);
            SetTextColor(paint.hdc, RGB(0, 0, 0));
        }

        MoveToEx(paint.hdc, 0, y + static_cast<int>(fEntryHeight) - fLineWidth, nullptr);
        LineTo(paint.hdc, entryWidth, y + static_cast<int>(fEntryHeight) - fLineWidth);

        const int textPosition = 5 + y;
        RECT textRect{0, textPosition, 0, textPosition + 24};
        DrawText(paint.hdc, imageDesc.title.c_str(), static_cast<int>(imageDesc.title.length()), &textRect,
                 DT_CALCRECT);
        SetBkMode(paint.hdc, TRANSPARENT);
        const int textOffset = (entryWidth - (textRect.right - textRect.left)) / 2;
        textRect.right += textOffset;
        textRect.left += textOffset;
        DrawText(paint.hdc, imageDesc.title.c_str(), static_cast<int>(imageDesc.title.length()), &textRect, DT_CENTER);

        if (imageDesc.bitmap != nullptr && imageDesc.mask != nullptr)
        {
            const auto bitmapHeader = imageDesc.bitmap->GetBitmapHeader();
            const int finalWidth    = std::min<int>(imageDestWidth, bitmapHeader.width);
            const int finalHeight   = std::min<int>(imageDestHeight, bitmapHeader.height);
            const int finalY        = finalHeight < imageDestHeight ? (static_cast<int>(fEntryHeight) - finalHeight) / 2
                                                                    : imagePosition;

            const HBITMAP mask   = reinterpret_cast<HBITMAP>(imageDesc.mask->GetNativeHandle());
            const HBITMAP bitmap = reinterpret_cast<HBITMAP>(imageDesc.bitmap->GetNativeHandle());
            HGDIOBJ oldBitmap    = SelectObject(memoryDc, mask);
            BitBlt(paint.hdc, (entryWidth - finalWidth) / 2, y + finalY, finalWidth, finalHeight, memoryDc, 0, 0,
                   SRCPAINT);
            SelectObject(memoryDc, bitmap);
            BitBlt(paint.hdc, (entryWidth - finalWidth) / 2, y + finalY, finalWidth, finalHeight, memoryDc, 0, 0,
                   SRCAND);
            SelectObject(memoryDc, oldBitmap);
        }

        ++currentEntry;
        y += static_cast<int>(fEntryHeight);
    }

    SelectObject(paint.hdc, fNativeState->verticalPen);
    MoveToEx(paint.hdc, 0, 0, nullptr);
    LineTo(paint.hdc, 0, y);
    SelectObject(paint.hdc, fNativeState->verticalPen2);
    MoveToEx(paint.hdc, 1, 0, nullptr);
    LineTo(paint.hdc, 1, y);
    SelectObject(paint.hdc, fNativeState->verticalPen);
    MoveToEx(paint.hdc, 2, 0, nullptr);
    LineTo(paint.hdc, 2, y);

    DeleteDC(memoryDc);
    EndPaint(window, &paint);
}
