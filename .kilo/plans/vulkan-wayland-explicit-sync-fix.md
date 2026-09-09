# Vulkan Wayland Fix - Implementation Plan

## Current Working State
- App shows welcome image with `__NV_DISABLE_EXPLICIT_SYNC=1` set
- This env var forces NVIDIA driver to use implicit sync on Wayland
- Without it: driver creates `wp_linux_drm_syncobj_surface_v1` but never calls `set_acquire_point` → compositor rejects buffer → blank window
- Manual syncobj surface creation failed (driver creates its own first, causing `surface_exists` fatal error)
- All manual syncobj code reverted. Clean state: env var + clean Vulkan renderer.

## Open Issues (Need Action)

### A. NVIDIA-only gating for `__NV_DISABLE_EXPLICIT_SYNC=1`
**File**: `Clients/OIViewer/Source/Main.cpp:87-93`
**Problem**: Env var is set unconditionally on all Linux GPUs. Should only apply to NVIDIA (`0x10de`).
**Fix**: Read `/sys/class/drm/card0/device/vendor`, only set env var if `0x10de`.
**Status**: Blocked by permissions — needs manual edit. Include `<fstream>` and `<string>`.

### B. Debug prints removed
Per-frame investigation prints were removed. The selected-GPU and invalid-index messages remain as permanent user-facing diagnostics.

### C. Generated Wayland files and CMakeLists.txt glob
- `OIVLib/Renderers/OIVVKRenderer/Generated/Wayland/linux-drm-syncobj-v1-client.c` and `.h` are unused
- `CMakeLists.txt` has `./Generated/Wayland/*.c` glob that should be removed

### D. CLI parameter to choose display adapter
**Problem**: No way to select which GPU to use on multi-GPU systems.
**Proposed**: `--gpu=<index>` or `--adapter=<index>` CLI flag, or enumerate adapters and pick by index/name.
**Scope**: Needs research into how OIV/LWS exposes adapter selection.

### E. Window background color mismatch
**Problem**: Window shows LWS background (#757578) instead of Vulkan clear color (#2D2F30). The first frame may show the wrong color before Vulkan renders.
**Fix candidates**: Match LWS background to renderer clear color, or ensure Vulkan presents immediately on show.

### F. Image is upside down
**Problem**: The welcome image (and likely all images) appears upside down in the Vulkan renderer.
**Fix candidate**: Check texture upload Y-flip, or adjust push constants / shader UV coords.

### G. Image is mirrored
**Problem**: The image appears mirrored (flipped horizontally).
**Fix candidate**: Check framebuffer coordinate system, push constants, or shader UV mapping.

## Removed / Reverted
- Manual syncobj surface creation (`CreateSyncobjSurface`, `wp_linux_drm_syncobj_manager_v1_get_surface`)
- Custom Wayland display error listener replacement
- LWS error flag manipulation
- `vkQueueWaitIdle` workaround
- `VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT` change → reverted to `OPAQUE_FD_BIT`
- `__NV_DISABLE_EXPLICIT_SYNC=1` removal from Main.cpp → restored

## Next Steps (in order)
1. Apply NVIDIA-only gating to Main.cpp (manual edit required)
2. Address CLI adapter selection (D)
3. Fix window background color (E)
4. Fix upside-down image (F)
5. Fix mirrored image (G)
6. Clean up debug prints (only when user explicitly asks)
