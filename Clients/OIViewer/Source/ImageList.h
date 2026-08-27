#pragma once

#include <LLUtils/Event.h>
#include <LLUtils/StringDefs.h>
#include <LWS/Bitmap.hpp>
#include <LWS/interfaces/backends.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

class ImageList
{
  public:

    struct ImageSelectionChangeArgs
    {
        int imageIndex = -1;
    };

    using ImageSelectionChangEvent = LLUtils::Event<void(const ImageSelectionChangeArgs&)>;

    struct ImageDesc
    {
        uint32_t index{};
        LLUtils::native_string_type title;
        LWS::BitmapSharedPtr bitmap;
        LWS::BitmapSharedPtr mask;
    };

    struct RGBAImageDesc
    {
        std::byte* buffer;
        uint32_t width;
        uint32_t height;
    };

    ImageList();
    ~ImageList();

    ImageList(const ImageList&)            = delete;
    ImageList& operator=(const ImageList&) = delete;

    ImageSelectionChangEvent ImageSelectionChanged;

    void SetTarget(LWS::Handle windowHandle);
    [[nodiscard]] int GetSelected() const;
    void Clear();
    void SetSelected(int selected);
    void MouseClick(int xPos, int yPos);
    void SetPos(int pos);
    [[nodiscard]] size_t GetNumberOfDisplayedElements();
    [[nodiscard]] size_t GetNumberOfElements() const;
    void Draw();
    void SetImage(const ImageDesc& imageDesc);

  private:

    struct NativeState;

    [[nodiscard]] std::optional<int32_t> GetViewportHeight() const;
    void PrepareImage(ImageDesc& imageDesc);
    void RequestRepaint(bool erase);

    static constexpr int fLineWidth = 2;
    uint32_t fEntryHeight           = 100;
    int fSelected                   = -1;
    int fPos                        = 0;
    std::vector<ImageDesc> fImages;
    std::unique_ptr<NativeState> fNativeState;
};
