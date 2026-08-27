#include "ViewerApplication.h"

#include <LLUtils/Exception.h>

namespace OIV
{
    void ViewerApplication::InitializeRawInput() {}

    int ViewerApplication::GetRawNavigationDirection() const
    {
        return 0;
    }

    void ViewerApplication::AddPlatformKeyBindings() {}

    bool ViewerApplication::handleKeyInput([[maybe_unused]] const LWS::AnyEvent& eventData)
    {
        LL_EXCEPTION_NOT_IMPLEMENT("Native key translation is not implemented on Linux");
    }

    std::intptr_t ViewerApplication::ClientWindwMessage(const LWS::AnyEvent& eventData)
    {
        if (std::holds_alternative<LWS::EventResize>(eventData))
        {
            fRefreshOperation.Begin();
            UpdateWindowSize();
            fRefreshOperation.End();
        }
        return 0;
    }

    bool ViewerApplication::HandleWinMessageEvent(const LWS::AnyEvent& eventData)
    {
        if (std::holds_alternative<LWS::EventKeyDown>(eventData) || std::holds_alternative<LWS::EventKeyUp>(eventData))
            return handleKeyInput(eventData);

        if (std::holds_alternative<LWS::EventMouseMove>(eventData))
            UpdateTexelPos();
        else if (std::holds_alternative<LWS::EventClose>(eventData))
            CloseApplication(false);
        else if (std::holds_alternative<LWS::EventFocusGained>(eventData))
            SetAppActive(true);
        else if (std::holds_alternative<LWS::EventFocusLost>(eventData))
            SetAppActive(false);
        else if (std::holds_alternative<LWS::EventPaint>(eventData) && !fIsFirstFrameDisplayed)
        {
            fIsFirstFrameDisplayed = true;
            AfterFirstFrameDisplayed();
        }
        return false;
    }

    void ViewerApplication::CloseApplication(bool closeToTray)
    {
        if (closeToTray)
            LL_EXCEPTION_NOT_IMPLEMENT("Close-to-tray is not implemented on Linux");
        fWindow.Destroy();
    }

    bool ViewerApplication::HandleMessages(const LWS::AnyEvent& eventData)
    {
        if (const auto* dragDropEvent = std::get_if<LWS::EventDragDropFile>(&eventData))
            return HandleFileDragDropEvent(*dragDropEvent);
        return HandleWinMessageEvent(eventData);
    }
}  // namespace OIV
