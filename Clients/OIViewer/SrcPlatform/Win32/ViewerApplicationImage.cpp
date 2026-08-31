#include "ViewerApplication.h"

#include <ImageUtil/ImageUtil.h>

#include <LLUtils/Buffer.h>
#include <LLUtils/Exception.h>
#include <LLUtils/PlatformUtility.h>
#include <LLUtils/StringUtility.h>

#include <Windows.h>
#include <shellapi.h>

#include <array>
#include <climits>
#include <cstring>

namespace OIV
{
    void ViewerApplication::DeleteOpenedFile(bool permanently)
    {
        const size_t stringLength = GetOpenedFileName().length();
        auto buffer               = std::make_unique<wchar_t[]>(stringLength + 2);
        std::memcpy(buffer.get(), GetOpenedFileName().c_str(), (stringLength + 1) * sizeof(wchar_t));
        buffer[stringLength + 1] = L'\0';

        SHFILEOPSTRUCT fileOperation{reinterpret_cast<HWND>(GetWindowHandle()),
                                     FO_DELETE,
                                     buffer.get(),
                                     nullptr,
                                     static_cast<FILEOP_FLAGS>(permanently ? 0 : FOF_ALLOWUNDO),
                                     FALSE,
                                     nullptr,
                                     nullptr};

        const auto fileNameToRemove = GetOpenedFileName();
        fRequestedFileForRemoval    = fileNameToRemove;
        if (SHFileOperation(&fileOperation) == 0)
            ProcessRemovalOfOpenedFile(fileNameToRemove);
    }

    void ViewerApplication::AddImageToControl(IMCodec::ImageSharedPtr image, uint16_t imageSlot, uint16_t totalImages)
    {
        auto bgraImage = IMUtil::ImageUtil::ConvertImageWithNormalization(image, IMCodec::TexelFormat::I_B8_G8_R8_A8,
                                                                          false);
        bgraImage = IMUtil::ImageUtil::Transform({IMUtil::AxisAlignedRotation::None, IMUtil::AxisAlignedFlip::Vertical},
                                                 bgraImage);

        LWS::BitmapBuffer bitmapBuffer{};
        bitmapBuffer.bitsPerPixel = bgraImage->GetBitsPerTexel();
        bitmapBuffer.rowPitch     = LLUtils::Utility::Align<uint32_t>(bgraImage->GetRowPitchInBytes(), sizeof(DWORD));
        LLUtils::Buffer colorBuffer(bgraImage->GetHeight() * bitmapBuffer.rowPitch);
        bitmapBuffer.height = bgraImage->GetHeight();
        bitmapBuffer.width  = bgraImage->GetWidth();

        LWS::BitmapBuffer maskBuffer{};
        maskBuffer.bitsPerPixel = 24;
        maskBuffer.height       = bgraImage->GetHeight();
        maskBuffer.width        = bgraImage->GetWidth();
        maskBuffer.rowPitch = LLUtils::Utility::Align<uint32_t>(maskBuffer.width * maskBuffer.bitsPerPixel / CHAR_BIT,
                                                                sizeof(DWORD));
        LLUtils::Buffer maskPixelsBuffer(maskBuffer.height * maskBuffer.rowPitch);

#pragma pack(push, 1)
        struct Color32
        {
            uint8_t red;
            uint8_t green;
            uint8_t blue;
            uint8_t alpha;
        };
        struct Color24
        {
            uint8_t red;
            uint8_t green;
            uint8_t blue;
        };
#pragma pack(pop)

        for (uint32_t line = 0; line < maskBuffer.height; ++line)
        {
            const uint32_t sourceOffset = line * bgraImage->GetRowPitchInBytes();
            const uint32_t colorOffset  = line * bitmapBuffer.rowPitch;
            const uint32_t maskOffset   = line * maskBuffer.rowPitch;
            for (size_t x = 0; x < maskBuffer.width; ++x)
            {
                Color24& destinationMask = reinterpret_cast<Color24*>(
                    reinterpret_cast<uint8_t*>(maskPixelsBuffer.data()) + maskOffset)[x];
                Color32& destinationImage  = reinterpret_cast<Color32*>(reinterpret_cast<uint8_t*>(colorBuffer.data()) +
                                                                        colorOffset)[x];
                const Color32& sourceColor = reinterpret_cast<const Color32*>(
                    reinterpret_cast<const uint8_t*>(bgraImage->GetBuffer()) + sourceOffset)[x];
                const uint8_t inverseAlpha = 0xFF - sourceColor.alpha;
                destinationMask.red        = sourceColor.alpha;
                destinationMask.green      = sourceColor.alpha;
                destinationMask.blue       = sourceColor.alpha;
                destinationImage.red       = sourceColor.red | inverseAlpha;
                destinationImage.green     = sourceColor.green | inverseAlpha;
                destinationImage.blue      = sourceColor.blue | inverseAlpha;
            }
        }

        bitmapBuffer.pixels = std::span<const std::byte>(colorBuffer.data(), colorBuffer.size());
        maskBuffer.pixels   = std::span<const std::byte>(maskPixelsBuffer.data(), maskPixelsBuffer.size());

        LLUtils::native_stringstream title;
        title << imageSlot + 1 << L'/' << totalImages << LLUTILS_TEXT("  ") << bitmapBuffer.width << LLUTILS_TEXT(" x ")
              << bitmapBuffer.height << LLUTILS_TEXT(" x ") << bitmapBuffer.bitsPerPixel << LLUTILS_TEXT(" BPP");
        fWindow.GetImageControl().GetImageList().SetImage({imageSlot, title.str(),
                                                           std::make_shared<LWS::Bitmap>(bitmapBuffer),
                                                           std::make_shared<LWS::Bitmap>(maskBuffer)});
    }

    ClipboardDataType ViewerApplication::PasteFromClipBoard()
    {
        ClipboardDataType clipboardType  = ClipboardDataType::None;
        const auto& [formatType, buffer] = fClipboardHelper.GetClipboardData();
        if (formatType == CF_DIB || formatType == CF_DIBV5)
        {
            const auto* bitmapInfo       = reinterpret_cast<const BITMAPINFO*>(buffer.data());
            const BITMAPINFOHEADER* info = &bitmapInfo->bmiHeader;
            const uint32_t rowPitch = LLUtils::Utility::Align<uint32_t>(info->biWidth * (info->biBitCount / CHAR_BIT),
                                                                        4);
            std::byte* bitmapBits   = const_cast<std::byte*>(reinterpret_cast<const std::byte*>(info) + info->biSize);
            if (info->biCompression == BI_BITFIELDS)
                bitmapBits += 3 * sizeof(DWORD);
            else if (info->biCompression != BI_RGB)
                LL_EXCEPTION(LLUtils::Exception::ErrorCode::NotImplemented,
                             std::string("Unsupported clipboard bitmap compression type: ") +
                                 std::to_string(info->biCompression));

            using namespace IMCodec;
            auto imageItem                     = std::make_shared<ImageItem>();
            ImageDescriptor& descriptor        = imageItem->descriptor;
            imageItem->itemType                = ImageItemType::Image;
            descriptor.height                  = info->biHeight;
            descriptor.width                   = info->biWidth;
            descriptor.texelFormatStorage      = info->biBitCount == 24 ? TexelFormat::I_B8_G8_R8
                                                                        : TexelFormat::I_B8_G8_R8_A8;
            descriptor.texelFormatDecompressed = descriptor.texelFormatStorage;
            descriptor.rowPitchInBytes         = rowPitch;
            const size_t bufferSize            = descriptor.rowPitchInBytes * descriptor.height;
            imageItem->data.Allocate(bufferSize);
            imageItem->data.Write(bitmapBits, 0, bufferSize);
            auto image = std::make_shared<Image>(imageItem, ImageItemType::Unknown);
            if (info->biCompression == BI_BITFIELDS)
                image = IMUtil::ImageUtil::Convert(image, TexelFormat::I_B8_G8_R8);
            image = IMUtil::ImageUtil::Transform({IMUtil::AxisAlignedRotation::None, IMUtil::AxisAlignedFlip::Vertical},
                                                 image);
            LoadOivImage(std::make_shared<OIVBaseImage>(ImageSource::Clipboard, image));
            clipboardType = ClipboardDataType::Image;
        }
        else if (formatType == CF_UNICODETEXT || formatType == CF_TEXT)
        {
            LLUtils::native_string_type text;
            if (formatType == CF_UNICODETEXT)
                text = reinterpret_cast<const wchar_t*>(buffer.data());
            else
                text = LLUtils::StringUtility::ToWString(reinterpret_cast<const char*>(buffer.data()));

            if (!text.empty())
            {
                LoadClipboardText(text);
                clipboardType = ClipboardDataType::Text;
            }
        }
        return clipboardType;
    }

    bool ViewerApplication::SetClipboardImage(IMCodec::ImageSharedPtr image)
    {
        auto clipboardCompatibleImage = IMUtil::ImageUtil::ConvertImageWithNormalization(
            image, IMCodec::TexelFormat::I_B8_G8_R8_A8, false);
        if (clipboardCompatibleImage == nullptr)
            return false;

        const uint32_t width       = clipboardCompatibleImage->GetWidth();
        const uint32_t height      = clipboardCompatibleImage->GetHeight();
        const uint8_t bitsPerPixel = clipboardCompatibleImage->GetBitsPerTexel();
        auto dibBuffer   = LLUtils::PlatformUtility::CreateDIB<1>(width, height, bitsPerPixel,
                                                                  clipboardCompatibleImage->GetRowPitchInBytes(),
                                                                  clipboardCompatibleImage->GetBuffer());
        auto dibV5Buffer = LLUtils::PlatformUtility::CreateDIB<5>(width, height, bitsPerPixel,
                                                                  clipboardCompatibleImage->GetRowPitchInBytes(),
                                                                  clipboardCompatibleImage->GetBuffer());
        const std::array clipboardData{
            LWS::ClipboardDataView{.format = CF_DIB, .data = {dibBuffer.data(), dibBuffer.size()}},
            LWS::ClipboardDataView{.format = CF_DIBV5, .data = {dibV5Buffer.data(), dibV5Buffer.size()}},
        };
        return fClipboardHelper.SetClipboardData(fWindow.GetHandle(), clipboardData) == LWS::ClipboardResult::Success;
    }

    void ViewerApplication::HandleReloadAction(ReloadAction action, const LLUtils::native_string_type& requestedFile)
    {
        if (action == ReloadAction::AskUser)
        {
            using namespace std::string_literals;
            const int result = MessageBox(reinterpret_cast<HWND>(fWindow.GetHandle()),
                                          (LLUTILS_TEXT("Reload the file: "s) + requestedFile).c_str(),
                                          LLUTILS_TEXT("File is changed outside of OIV"), MB_YESNO);
            action           = fFileReloadPolicy.ConfirmReload(result == IDYES);
        }
        if (action == ReloadAction::RequestNow && fBrowseSessionController != nullptr)
            fBrowseSessionController->RequestCurrentFileReload();
    }
}  // namespace OIV
