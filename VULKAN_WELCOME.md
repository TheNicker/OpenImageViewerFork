# Vulkan Wayland Welcome Message Investigation

## Problem
When starting OIViewer with Vulkan renderer and no image file, the welcome message
("Press F1 for help") is not displayed, while it works with OpenGL renderer.
User confirms: "there is still no welcome message" despite console showing DrawImage + Present complete.

## Investigation Notes (Latest)
- Console output shows rendering IS happening: DrawImage with imageSize=1017x222, Present result=0
- OpenGL debug output shows NO renderer-specific debug (debug only added to VKRenderer)
- Vulkan renders but visual output is not visible on screen
- An NVIDIA explicit-sync override was tried during the initial investigation; OIViewer no longer sets it.
- Need to verify with WAYLAND_DEBUG=1 whether buffers are actually committed to surface

## Next Steps
1. Run with WAYLAND_DEBUG=1 to trace Wayland protocol communication
2. Check if the Wayland surface receives buffers via wl_surface_attach + wl_surface_commit
3. If buffer is not committed, need a different approach to fix the explicit sync issue
4. Compare OpenGL vs Vulkan: does OpenGL use EGL presentation (eglSwapBuffers) which handles
   Wayland protocol differently than Vulkan's vkQueuePresentKHR

## Intel GPU on Wayland Investigation

### Reported failure
The earlier Intel Vulkan run with `--gpu=1` reported:
```
wp_linux_drm_syncobj_surface_v1#19: error 4: explicit sync is used, but no acquire point is set
```
The earlier NVIDIA environment override did not control Intel's driver and has now been removed.

### Application-side startup issue
OIViewer initialized the renderer before disabling LWS background erasure. Erasure was disabled
only in `PostInitOperations`, reached from the first paint event. On Wayland, LWS paints the
background by attaching and committing a shared-memory buffer before dispatching that event.
This leaves a startup path where LWS commits its background buffer after Vulkan has taken
ownership of the same surface and may have enabled explicit synchronization.

The [linux-drm-syncobj protocol](https://wayland.app/protocols/linux-drm-syncobj-v1)
requires acquire and release points whenever a non-null buffer is newly attached to a surface
with an active syncobj object. Error 4 means the acquire point was not set for that commit;
it does not mean an acquire fence was set but never signaled. The client sets the acquire point,
not the compositor. The earlier conclusion that Intel could not be fixed in OIV was unsupported.

### Change and verification
`ViewerApplication::Init` now disables canvas background erasure immediately before renderer
initialization, preventing subsequent LWS background attachments to the renderer-owned surface.
The automatic `__NV_DISABLE_EXPLICIT_SYNC` override and NVIDIA device detection have been removed.

Local verification (2026-09-09): Windows and Linux viewer builds passed. Before/after WSLg
traces with Mesa llvmpipe showed one LWS background attachment to the Vulkan canvas before
the change and zero afterward for empty, image, and folder startup; Vulkan still presented
in every case. Each Linux run stayed alive for six seconds until stopped by the probe.
The corresponding Windows startup runs stayed alive for four seconds before being closed.
The focused renderer/platform tests passed (74 assertions in 12 test cases).
These protocol traces exercise the startup handoff, but this environment has no Intel DRM
device and does not validate the hardware explicit-sync path.

Intel/Mesa/KWin confirmation requires a run on the affected system. A `WAYLAND_DEBUG=1` trace
should show renderer buffer attachments with acquire/release points and no LWS background
buffer attachments on that surface after the syncobj object is created.

## Known Issue: Background Color Mismatch

### Problem
On Vulkan + Wayland, the window background appears as `#B4B4B6` instead of the expected `#2D2D30` that OpenGL shows. The color cannot be reliably checked in Wayland because the window appears transparent during the startup flash.

### Root Cause
On Wayland, the compositor shows its default surface color before the first Vulkan frame is presented. OpenGL presents its first frame faster, so the mismatch is not visible. Vulkan on Wayland has a slight delay, causing the compositor's default color to show momentarily.

### Status
- LWS window background is set to `#2D2D30` in `ViewerApplication.cpp:115-116`
- Vulkan clear color is also set to `#2D2D30` in `VKRenderer.cpp:227`
- Both match, but the Wayland compositor still shows its default color before the first frame
- This is a **timing/surface creation issue** on Wayland, not a color mismatch in the code

### Workaround
None currently. The issue resolves once the first Vulkan frame is presented.

## Item F: Image Upside Down Investigation

### Finding
Vulkan's positive-height viewport maps NDC Y directly to framebuffer Y, so applying the OpenGL texture-coordinate flip inverted the complete rendered frame. The Vulkan branch now uses the generated quad coordinates without an additional Y flip:
- OpenGL shader: `coords.y = 1.0 - coords.y;`
- Vulkan shader: no Y flip

Image data is stored top-to-bottom in memory. With the Vulkan viewport transform:
- Top-left vertex → UV (0, 0) → top of image (row 0) → correct orientation
- Bottom-left vertex → UV (0, 1) → bottom of image → correct orientation

### Conclusion
The Vulkan-only Y flip was removed. Images, overlays, and selection coordinates now share the correct top-to-bottom framebuffer orientation.

### Resolved

## Item G: Image Mirrored Investigation

### Current State
No horizontal flip is applied in either the vertex shader or fragment shader. The UV coordinates map:
- Left edge of quad → UV x = 0 (left of image)
- Right edge of quad → UV x = 1 (right of image)

### Comparison with OpenGL
OpenGL renderer uses identical UV mapping. No horizontal flip in either renderer.

### Conclusion
The image is **not mirrored** in the current Vulkan renderer. No code changes needed.

### Next Steps
The original plan items E, F, G (background color, upside-down, mirrored) appear to be already resolved or not reproducible with the current code. The remaining work is:
- Intel GPU on Wayland (startup ownership fix needs confirmation on the affected hardware)
- GPU selection UX (already implemented)
