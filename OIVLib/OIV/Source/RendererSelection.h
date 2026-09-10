#pragma once
#include <Interfaces/IRenderer.h>

namespace OIV
{
    inline constexpr std::size_t MaxRendererCount = 3;
    struct RendererBackend
    {
        RendererInfo info;
        IRendererSharedPtr (*create)();
    };

    // Backends are in compiled API order. Only startup may choose a new renderer; each failed
    // or deferred instance is destroyed before another factory is called. dataPath is a base
    // directory; the backend name is appended for each synchronous initialization call.
    IRendererSharedPtr SelectRenderer(std::span<const RendererBackend> backends, const RendererOptions& options,
                                      const OIV_RendererInitializationParams& params);
    std::span<const RendererBackend> GetRendererBackends();
}  // namespace OIV
