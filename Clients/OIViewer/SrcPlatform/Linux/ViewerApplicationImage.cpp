#include "ViewerApplication.h"

#include <LLUtils/Exception.h>

namespace OIV
{
    void ViewerApplication::DeleteOpenedFile([[maybe_unused]] bool permanently)
    {
        LL_EXCEPTION_NOT_IMPLEMENT("File deletion is not implemented on Linux");
    }

    void ViewerApplication::AddImageToControl([[maybe_unused]] IMCodec::ImageSharedPtr image,
                                              [[maybe_unused]] uint16_t imageSlot,
                                              [[maybe_unused]] uint16_t totalImages)
    {
        LL_EXCEPTION_NOT_IMPLEMENT("Native subimage presentation is not implemented on Linux");
    }

    ClipboardDataType ViewerApplication::PasteFromClipBoard()
    {
        LL_EXCEPTION_NOT_IMPLEMENT("Clipboard access is not implemented on Linux");
    }

    bool ViewerApplication::SetClipboardImage([[maybe_unused]] IMCodec::ImageSharedPtr image)
    {
        LL_EXCEPTION_NOT_IMPLEMENT("Clipboard access is not implemented on Linux");
    }

    void ViewerApplication::HandleReloadAction(ReloadAction action,
                                               [[maybe_unused]] const LLUtils::native_string_type& requestedFile)
    {
        if (action == ReloadAction::AskUser)
            LL_EXCEPTION_NOT_IMPLEMENT("Native reload confirmation is not implemented on Linux");
        if (action == ReloadAction::RequestNow && fBrowseSessionController != nullptr)
            fBrowseSessionController->RequestCurrentFileReload();
    }
}  // namespace OIV
