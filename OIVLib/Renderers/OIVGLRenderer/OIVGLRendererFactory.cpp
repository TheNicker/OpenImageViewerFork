#include "OIVGLRendererFactory.h"
#include "OIVGLRenderer.h"

namespace OIV
{
    IRendererSharedPtr GLRendererFactory::Create()
    {
        return std::make_shared<OIVGLRenderer>();
    }
}  // namespace OIV
