#include "ViewerApplication.h"

#include <LLUtils/Exception.h>

#include <gio/gio.h>

#include <string>

namespace OIV
{
    void ViewerApplication::DeleteOpenedFile(bool permanently)
    {
        const auto fileNameToRemove = GetOpenedFileName();
        GFile* file                 = g_file_new_for_path(fileNameToRemove.c_str());
        GError* error               = nullptr;
        fRequestedFileForRemoval    = fileNameToRemove;
        const gboolean removed      = permanently ? g_file_delete(file, nullptr, &error)
                                                  : g_file_trash(file, nullptr, &error);
        g_object_unref(file);

        if (removed != FALSE)
        {
            ProcessRemovalOfOpenedFile(fileNameToRemove);
        }
        else
        {
            fRequestedFileForRemoval.clear();
            std::string message = permanently ? "Unable to delete file" : "Unable to move file to trash";
            if (error != nullptr)
            {
                message += ": ";
                message += error->message;
                g_error_free(error);
            }
            LL_EXCEPTION(LLUtils::Exception::ErrorCode::RuntimeError, message);
        }
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
