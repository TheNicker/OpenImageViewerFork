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
