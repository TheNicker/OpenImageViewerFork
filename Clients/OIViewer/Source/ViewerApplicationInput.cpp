#include "ViewerApplication.h"

#include <OIVAppCore/ViewerPresentationPolicy.h>

#include <filesystem>

namespace OIV
{
    void ViewerApplication::SetTopMostUserMesage()
    {
        SetUserMessage(ViewerPresentationPolicy::FormatTopMostMessage(fTopMostCounter),
                       static_cast<GroupID>(UserMessageGroups::WindowOnTop),
                       MessageFlags::Interchangeable | MessageFlags::ManualRemove);
    }

    bool ViewerApplication::GetAppActive() const
    {
        return fIsActive;
    }

    void ViewerApplication::SetAppActive(bool active)
    {
        if (active == fIsActive)
            return;

        fIsActive = active;
        if (fIsActive && fFileReloadPolicy.HasPendingReloadFor(GetOpenedFileName()))
            PerformReloadFile(GetOpenedFileName());
        else
            UpdateTitle();
    }

    void ViewerApplication::ProcessTopMost()
    {
        if (fTopMostCounter <= 0)
            return;

        --fTopMostCounter;
        if (fTopMostCounter == 0)
        {
            fTimerTopMostRetention.SetInterval(0);
            fWindow.SetAlwaysOnTop(false);
            fMessageManager->RemoveGroup(static_cast<GroupID>(UserMessageGroups::WindowOnTop));
        }
        else
        {
            SetTopMostUserMesage();
        }
    }

    bool ViewerApplication::HandleFileDragDropEvent(const LWS::EventDragDropFile& eventDragDropFile)
    {
        const LLUtils::native_string_type normalizedPath =
            std::filesystem::path(eventDragDropFile.fileName).lexically_normal().native();
        if (!LoadFileOrFolder(normalizedPath,
                              IMCodec::PluginTraverseMode::AnyPlugin | IMCodec::PluginTraverseMode::AnyFileType))
            return false;

        fWindow.SetForground();
        return true;
    }

    bool ViewerApplication::HandleClientWindowMessages(const LWS::AnyEvent& eventData)
    {
        return ClientWindwMessage(eventData) != 0;
    }
}  // namespace OIV
