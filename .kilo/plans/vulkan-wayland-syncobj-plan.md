# Vulkan Wayland Explicit Sync - Root Cause Analysis & Fix Plan

## Confirmed Facts

1. **Error**: `wp_linux_drm_syncobj_surface_v1#33: error 4: explicit sync is used, but no acquire point is set`
2. **Window does not appear** but process does not crash
3. **OpenGL renderer works** on the same system (EGL handles explicit sync internally)
4. **Vulkan renderer uses `vkQueuePresentKHR`** — the NVIDIA driver handles all Wayland protocol interactions internally
5. **LWS manages the `wl_surface`** — `fWlSurface` is private, not exposed to OIV
6. **OIV receives `wl_surface*` as `window` parameter** in `VKContext::CreateSurface()`

## Root Cause

The NVIDIA proprietary Vulkan driver on Wayland has a **driver bug/limitation** with explicit sync:

- The driver automatically creates a `wp_linux_drm_syncobj_surface_v1` for the `wl_surface` when presenting via `vkQueuePresentKHR`
- The driver attaches explicit sync metadata (DMA-BUF + semaphore FD) to the `wl_buffer`
- The compositor detects explicit sync, validates the syncobj surface, and expects `set_acquire_point` / `set_release_point` before each commit
- **The NVIDIA driver does NOT properly signal the acquire point** on the syncobj surface it created
- Result: compositor rejects the buffer with `error 4`

This is confirmed by NVIDIA's own egl-wayland PR history showing explicit sync for Vulkan Wayland WSI was implemented in driver 560+ and required kernel 6.8+ with DRM fixes. Earlier driver versions have workarounds like `vkQueueWaitIdle` before presenting, but these are incomplete.

## Why Previous "Fixes" Failed

- Adding `VK_KHR_external_semaphore_fd` and `VkExportSemaphoreCreateInfo` only makes semaphores exportable — it does NOT wire them into the Wayland explicit sync protocol
- The Vulkan driver handles `vkQueuePresentKHR` internally; the application has no opportunity to call `wp_linux_drm_syncobj_surface_v1_set_acquire_point`
- We cannot create our own syncobj surface because the protocol forbids it if one already exists (`surface_exists` error), and we don't have a handle to the driver-created one

## Feasible Solutions (Ranked)

### Option A: Environment Variable Workaround (Fastest to try)

Set `__NV_DISABLE_EXPLICIT_SYNC=1` before launching OIViewer. This is documented in NVIDIA's egl-wayland repository as disabling explicit sync in the driver. May also affect native Vulkan WSI.

**Risk**: May not work for native Vulkan (only tested with egl-wayland). May cause fallback to implicit sync with potential performance/visual issues.

### Option B: `vkQueueWaitIdle` Before Present (Driver Workaround)

Add `vkQueueWaitIdle(fPresentQueue)` immediately before `vkQueuePresentKHR`. This is the workaround NVIDIA's driver uses internally when explicit sync is unavailable — it ensures the buffer is fully rendered before presenting, allowing the driver to fall back to implicit sync.

**Risk**: Significant performance cost (stalls the GPU pipeline). But ensures correctness.

### Option C: EGL + Vulkan Interop (Most Robust)

Instead of using `vkQueuePresentKHR` directly:

1. Use EGL to create the Wayland surface and manage presentation
1. Use `VK_KHR_wayland_surface` + EGL interop to render Vulkan frames
1. EGL handles explicit sync properly (NVIDIA's egl-wayland supports it)

**Risk**: Major architectural change. Requires EGL initialization, cross-API synchronization, and significant refactoring of VKContext/VKRenderer.

### Option D: Manual Presentation via LWS (Complex but Application-Controlled)

Bypass `vkQueuePresentKHR` entirely:

1. Acquire swapchain image manually
1. Render to it
1. Export the image as DMA-BUF + semaphore FD
1. Create `wl_buffer` from DMA-BUF using `wp_linux_dmabuf_v1`
1. Create our own `wp_linux_drm_syncobj_surface_v1` (if driver hasn't already)
1. Set acquire/release points manually
1. Use LWS's internal `wl_surface_attach` + `wl_surface_commit`
1. Handle `wl_buffer.release` events

**Risk**: Extremely complex. LWS doesn't expose API for presenting arbitrary `wl_buffer`s. Would require modifying LWS or bypassing it entirely. Race conditions with LWS's own surface management.

## Recommended Plan

**Step 1**: Verify the environment variable workaround (Option A)

- Run with `__NV_DISABLE_EXPLICIT_SYNC=1 ./bin/OIViewer --renderer=vulkan ...`
- If window appears, this is the immediate fix

**Step 2**: If env var doesn't work, add `vkQueueWaitIdle` before present (Option B)

- Modify `VKRenderer::Redraw()` to call `vkQueueWaitIdle` before `vkQueuePresentKHR`
- Test if window appears (with performance cost)

**Step 3**: If both fail, investigate driver version and kernel requirements

- Check `nvidia-smi` and kernel version
- NVIDIA driver 560+ and kernel 6.8+ are required for proper Vulkan Wayland explicit sync
- If system doesn't meet requirements, document this as a system requirement

**Step 4**: Long-term, implement EGL + Vulkan interop (Option C) for a proper fix

- This aligns with how the OpenGL renderer works
- EGL handles Wayland protocol details correctly
- Requires significant refactoring but is the correct architectural solution

## What We Need to Know Before Proceeding

1. **NVIDIA driver version**: `nvidia-smi` or check in settings
2. **Kernel version**: `uname -r`
3. **Compositor**: Mutter (GNOME) or KWin (KDE)? Version?
4. **Does `__NV_DISABLE_EXPLICIT_SYNC=1` fix it?** (test needed)

## Questions for User

1. What NVIDIA driver version are you running?
2. What kernel version?
3. What compositor (Mutter/KWin) and version?
4. Are you able to test with `__NV_DISABLE_EXPLICIT_SYNC=1`?
