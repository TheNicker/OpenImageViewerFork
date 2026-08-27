#pragma once

#include "OIVCommands.h"

#include <cstddef>
#include <cstdint>

namespace OIV
{
    class IViewerRenderPort
    {
      public:

        virtual ~IViewerRenderPort() = default;

        virtual void Initialize(std::size_t canvasHandle, void* nativeDisplay = nullptr)         = 0;
        virtual void ResumePresentation()                                                        = 0;
        virtual ResultCode Refresh()                                                             = 0;
        virtual void SetSelectionRect(const LLUtils::RectI32& rect)                              = 0;
        virtual void ClearSelectionRect()                                                        = 0;
        virtual ResultCode SetColorExposure(const OIV_CMD_ColorExposure_Request& exposure)       = 0;
        virtual ResultCode SetTexelGrid(const CmdRequestTexelGrid& grid)                         = 0;
        virtual ResultCode SetClientSize(uint16_t width, uint16_t height)                        = 0;
        virtual ResultCode RegisterCallbacks(const OIV_CMD_RegisterCallbacks_Request& callbacks) = 0;
    };

    class OivRenderGateway final : public IViewerRenderPort
    {
      public:

        enum class PresentationState
        {
            Ready,
            Deferred,
        };

        explicit OivRenderGateway(PresentationState state = PresentationState::Ready)
            : fPresentationReady(state == PresentationState::Ready)
        {
        }

        void Initialize(std::size_t canvasHandle, void* nativeDisplay = nullptr) override
        {
            OIVCommands::Init(canvasHandle, nativeDisplay);
        }

        ResultCode Refresh() override
        {
            if (!fPresentationReady)
            {
                fRefreshPending = true;
                return RC_Success;
            }
            return OIVCommands::Refresh();
        }

        void ResumePresentation() override
        {
            fPresentationReady = true;
            if (fRefreshPending)
            {
                fRefreshPending = false;
                OIVCommands::Refresh();
            }
        }

        void SetSelectionRect(const LLUtils::RectI32& rect) override { OIVCommands::SetSelectionRect(rect); }

        void ClearSelectionRect() override { OIVCommands::CancelSelectionRect(); }

        ResultCode SetColorExposure(const OIV_CMD_ColorExposure_Request& exposure) override
        {
            OIV_CMD_ColorExposure_Request request = exposure;
            return OIVCommands::ExecuteCommand(OIV_CMD_ColorExposure, &request, &OIVCommands::NullCommand);
        }

        ResultCode SetTexelGrid(const CmdRequestTexelGrid& grid) override
        {
            CmdRequestTexelGrid request = grid;
            return OIVCommands::ExecuteCommand(CE_TexelGrid, &request, &OIVCommands::NullCommand);
        }

        ResultCode SetClientSize(uint16_t width, uint16_t height) override
        {
            CmdSetClientSizeRequest request{width, height};
            return OIVCommands::ExecuteCommand(CMD_SetClientSize, &request, &OIVCommands::NullCommand);
        }

        ResultCode RegisterCallbacks(const OIV_CMD_RegisterCallbacks_Request& callbacks) override
        {
            OIV_CMD_RegisterCallbacks_Request request = callbacks;
            return OIVCommands::ExecuteCommand(OIV_CMD_RegisterCallbacks, &request, &OIVCommands::NullCommand);
        }

      private:

        bool fPresentationReady;
        bool fRefreshPending{};
    };
}  // namespace OIV
