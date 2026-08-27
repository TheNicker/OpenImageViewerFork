#include "CommandHandlerInit.h"
#include "../CommandProcessor.h"
#include "../../OIV.h"
#include "../../ApiGlobal.h"

namespace OIV
{
    ResultCode CommandHandlerInit::ExecuteImpl(const void* request, [[maybe_unused]] const std::size_t requestSize,
                                               [[maybe_unused]] void* response,
                                               [[maybe_unused]] const std::size_t responseSize)
    {
        const auto* dataInit = static_cast<const CmdDataInit*>(request);
        ApiGlobal::sPictureRenderer->SetParent(dataInit->parentHandle, dataInit->nativeDisplay);
        ApiGlobal::sPictureRenderer->Init();
        return RC_Success;
    }

}  // namespace OIV
