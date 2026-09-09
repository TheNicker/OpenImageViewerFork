# Vulkan Renderer Implementation Plan

## Overview

Add a new `OIVVKRenderer` backend to OIV alongside the existing OpenGL and D3D11 renderers. The Vulkan renderer must implement the same `IRenderer` interface, follow the existing factory pattern, and plug into the compile-time renderer selection in `OIV::CreateBestRenderer()`.

## Existing Architecture Summary

| Component | Location | Purpose |
|-----------|----------|---------|
| `IRenderer` | `OIVLib/OIV/Include/Interfaces/IRenderer.h` | Pure virtual interface (9 methods) |
| `IRenderable` | `OIVLib/OIV/Include/Interfaces/IRenderable.h` | Renderable image/overlay interface |
| `ViewParameters` | `OIVLib/OIV/Include/Interfaces/IRendererDefs.h` | Viewport size, transparency colors, grid flag |
| `OIV_RendererInitializationParams` | `OIVLib/OIV/Include/Defs.h:143` | `container` (HWND/native window), `nativeDisplay`, `dataPath` |
| `OIV::CreateBestRenderer()` | `OIVLib/OIV/Source/OIV.cpp:101` | Compile-time selection via `#if` on `OIV_BUILD_RENDERER_*` |
| Shared shaders | `OIVLib/OIV/Resources/Programs/*.shader` | Multi-API shader sources with `#if defined(HLSL) || defined(D3D11)` / `#elif GLSL` |
| D3D11 renderer | `OIVLib/Renderers/OIVD3D11Renderer/` | Reference for modern renderer structure |
| GL renderer | `OIVLib/Renderers/OIVGLRenderer/` | Reference for cross-platform context (Win32/Wayland/X11) |

## Key Decisions Already Made

1. **Follow the existing pattern** — new `OIVVKRenderer` directory under `OIVLib/Renderers/`, factory class, `IRenderer` implementation.
2. **Vulkan only on Windows and Linux** — matching the GL renderer's platform scope.
3. **Shader source** — add a `#elif defined(VULKAN)` / `#elif defined(VK)` branch to the existing `.shader` files, or compile GLSL to SPIR-V at build time. Recommend adding Vulkan branches to `.shader` files first, then transpiling to SPIR-V.
4. **Use GLSL → SPIR-V via `glslc`** — at build time, compile the GLSL portions of `.shader` files to `.spv` and load them at runtime, similar to D3D11's shader caching.
5. **Cross-platform windowing** — support Win32 (VK_KHR_win32_surface), X11 (VK_KHR_xlib_surface), and Wayland (VK_KHR_wayland_surface) via a `VKContext` abstraction, modeled after `GLContext`.

## Implementation Tasks

### Phase 1: Build System

1. **Add CMake option** in `OIVLib/CMakeLists.txt`:
   ```
   option(OIV_BUILD_RENDERER_VK "Build Vulkan renderer" OFF)
   ```

2. **Create** `OIVLib/Renderers/OIVVKRenderer/CMakeLists.txt`:
   - Set target name `OIVVKRenderer`
   - Find and link Vulkan SDK (`Vulkan::Vulkan` via `find_package(Vulkan REQUIRED)`)
   - Fetch or find `glslc`/`glslangValidator` for shader compilation (prefer system `glslc` from Vulkan SDK)
   - Add compile definitions for surface extensions based on platform (`VK_USE_PLATFORM_WIN32_KHR`, `VK_USE_PLATFORM_XLIB_KHR`, `VK_USE_PLATFORM_WAYLAND_KHR`)
   - Build as static library

3. **Update** `OIVLib/Renderers/CMakeLists.txt`:
   ```
   if(OIV_BUILD_RENDERER_VK)
       add_subdirectory(OIVVKRenderer)
   endif()
   ```

### Phase 2: Shader Infrastructure

4. **Add Vulkan branches** to existing `.shader` files in `OIVLib/OIV/Resources/Programs/`:
   - `QuadVP.shader` — add GLSL vertex shader under `#elif defined(VULKAN)` or `#elif GLSL` (Vulkan uses the same GLSL as GL, but needs SPIR-V compilation)
   - `QuadFP.shader` — add GLSL fragment shader entry point
   - `QuadSelectionFP.shader` — add GLSL fragment shader entry point
   - `QuadSimpleFP.shader` — add GLSL fragment shader entry point
   - Note: The D3D11 path already has both HLSL and GLSL. Vulkan will use the GLSL path and compile it to SPIR-V.

5. **Create shader build step**:
   - At CMake configure time, compile `.shader` GLSL portions to `.spv` files into a cache directory (matching D3D11's `ShaderCache/` pattern).
   - Alternatively, compile at `Init()` time on first run and cache the `.spv` blobs in `dataPath/ShaderCache/`.

### Phase 3: Vulkan Context / Surface

6. **Create** `OIVLib/Renderers/OIVVKRenderer/VKContext.h` and `VKContext.cpp`:
   - Mirror `GLContext`'s platform abstraction
   - `Init(windowHandle, nativeDisplay)` — create `VkInstance`, `VkSurfaceKHR`, pick physical device, create logical device, create swap chain, get queue
   - `Resize(width, height)` — recreate swap chain
   - `SwapBuffers()` — `vkQueuePresentKHR`
   - Platform-specific implementations:
     - `VKContextWin32.cpp` — `VK_KHR_win32_surface`
     - `VKContextX11.cpp` — `VK_KHR_xlib_surface`
     - `VKContextWayland.cpp` — `VK_KHR_wayland_surface`

7. **Device selection logic**:
   - Prefer discrete GPU (`VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU`)
   - Fall back to integrated GPU
   - Require `VK_KHR_swapchain` extension
   - Use a single graphics+present queue family if possible, else use separate queues with a barrier

### Phase 4: Resource Management

8. **Create** `OIVVKRenderer/VKTexture.h`:
   - Wraps `VkImage`, `VkDeviceMemory`, `VkImageView`
   - `Create(width, height, format, initialData)`
   - `TransitionToShaderRead()` — image layout barrier
   - `Bind(slot)` — bind descriptor set or update descriptor set

9. **Create** `OIVVKRenderer/VKPipeline.h`:
   - Wraps `VkPipeline`, `VkPipelineLayout`, descriptor set layout
   - Compile SPIR-V shaders at build time or load cached `.spv`
   - Create `VkShaderModule` from SPIR-V
   - Create `VkPipeline` with `VkGraphicsPipelineCreateInfo`
   - Fixed-function state: rasterization, blend state (alpha blend), depth stencil (none), viewport (dynamic)

10. **Create** `OIVVKRenderer/VKBuffer.h`:
    - Wraps `VkBuffer`, `VkDeviceMemory`
    - Uniform buffer for shader constants (matching D3D11's constant buffer layout)
    - Map/unmap for CPU writes

### Phase 5: Renderer Implementation

11. **Create** `OIVVKRenderer/VKRenderer.h` and `VKRenderer.cpp`:
    - Internal rendering engine (like D3D11's `D3D11Renderer`)
    - Manages device, swap chain, pipelines, render passes, command buffers
    - `Init(params)` → create context, create render pass, create pipelines, create command pool, allocate command buffer
    - `Redraw()` → acquire swap chain image, begin render pass, draw main images, draw selection rect, draw overlays, end render pass, present
    - `DrawImage(entry)` → update uniform buffer, bind pipeline, bind texture, draw 4 vertices (triangle strip)
    - Handle back buffer resize via `ResizeBackBuffer()`

12. **Create** `OIVVKRenderer/OIVVKRenderer.h` and `OIVVKRenderer.cpp`:
    - Implements `IRenderer` (thin wrapper delegating to `VKRenderer`, mirroring `OIVD3D11Renderer`)
    - `AddRenderable` / `RemoveRenderable` — maintain `std::map<uint32_t, ImageEntry>`
    - `DrawImage` — lazy texture upload when `GetIsImageDirty()` is true, convert to RGBA8 if needed

13. **Create** `OIVVKRenderer/OIVVKRendererFactory.h` and `OIVVKRendererFactory.cpp`:
    - `static IRendererSharedPtr Create();` returning `std::make_shared<OIVVKRenderer>()`

### Phase 6: Integration

14. **Update** `OIVLib/OIV/Source/OIV.cpp`:
    - Add `#if OIV_BUILD_RENDERER_VK == 1` include for `OIVVKRendererFactory.h`
    - Update `CreateBestRenderer()` to prefer Vulkan on Windows (or add as additional option)
    - Update `RendererName()` to return `"Vulkan"` when selected

15. **Update** `OIVLib/OIV/Source/OIV.cpp` `CreateBestRenderer()`:
    - On Windows: D3D11 > Vulkan > GL > Null
    - On Linux: GL > Vulkan > Null (or Vulkan > GL if preferred; recommend GL first for maturity)

### Phase 7: Validation

16. **Build verification**:
    - `cmake -DOIV_BUILD_RENDERER_VK=ON ..` succeeds on Windows and Linux
    - Existing GL and D3D11 builds still succeed with Vulkan disabled

17. **Runtime verification**:
    - Load a simple image, verify it renders
    - Test pan/zoom (via `IRenderable::GetScale` / `GetPosition`)
    - Test selection rectangle
    - Test overlay rendering
    - Test window resize
    - Test transparency checkerboard and color correction

## File Inventory (New Files)

```
OIVLib/Renderers/OIVVKRenderer/
├── CMakeLists.txt
├── Include/
│   └── OIVVKRendererFactory.h
├── Source/
│   ├── OIVVKRenderer.h
│   ├── OIVVKRenderer.cpp
│   ├── OIVVKRendererFactory.cpp
│   ├── VKContext.h
│   ├── VKContext.cpp
│   ├── VKContextWin32.cpp
│   ├── VKContextX11.cpp
│   ├── VKContextWayland.cpp
│   ├── VKRenderer.h
│   ├── VKRenderer.cpp
│   ├── VKTexture.h
│   ├── VKTexture.cpp
│   ├── VKPipeline.h
│   ├── VKPipeline.cpp
│   ├── VKBuffer.h
│   ├── VKBuffer.cpp
│   └── VKCommon.h
```

## Modified Files

- `OIVLib/CMakeLists.txt` — add `OIV_BUILD_RENDERER_VK` option
- `OIVLib/Renderers/CMakeLists.txt` — add Vulkan subdirectory
- `OIVLib/OIV/Source/OIV.cpp` — add Vulkan factory include and selection logic
- `OIVLib/OIV/Resources/Programs/*.shader` — add Vulkan/GLSL branches

## Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| Vulkan SDK dependency | Use `find_package(Vulkan REQUIRED)`; document minimum version (1.3.0 recommended, 1.1.0 minimum) |
| Shader format divergence | Keep `.shader` files as single source of truth; add Vulkan GLSL branches alongside existing HLSL/GLSL |
| Platform surface creation complexity | Reuse `GLContext`'s platform detection pattern; isolate platform code in separate `.cpp` files |
| Descriptor set management | Start with a simple per-frame dynamic uniform buffer and a single texture descriptor set |
| Validation layer overhead | Enable in debug builds only; use `VK_LAYER_KHRONOS_validation` |

## Open Questions (To Resolve Before Implementation)

1. **Shader compilation strategy**: Should `.spv` files be pre-built at CMake time, or compiled lazily at first run and cached in `dataPath/ShaderCache/` (like D3D11)?
   - **Recommended**: Lazy compile + cache, matching D3D11's pattern, to avoid requiring `glslc` at build time.

2. **Vulkan version target**: 1.1 (widely supported) or 1.3 (simpler, modern)?
   - **Recommended**: Target 1.1 minimum, use 1.3 features if available. Use `vkEnumerateInstanceVersion` to detect.

3. **Render pass complexity**: Single render pass with dynamic color attachment (swap chain image), or use dynamic rendering (`VK_KHR_dynamic_rendering`)?
   - **Recommended**: Traditional render pass + framebuffer for maximum compatibility, recreate on swap chain change.

4. **Should Vulkan be the default on any platform?**
   - **Recommended**: No. Keep D3D11 as default on Windows, GL as default on Linux. Add Vulkan as an opt-in CMake option.
