# Item D Follow-up: GPU Selection UX

## Requirement
Print the selected GPU to stdout/stderr in ALL cases — default, in-range, out-of-range, and any fallback. This is a permanent user-facing message, not a debug print.

## Current Behavior
- `--gpu=0` → NVIDIA selected, works
- `--gpu=1` → Intel selected, fails with Wayland explicit-sync error 4
- `--gpu=2+` → out of range, silently falls back to discrete-GPU preference (NVIDIA), so it "works" but user doesn't know what happened
- No `--gpu` → same fallback to discrete GPU, no message

## Required Changes

### 1. Add permanent GPU selection log in `VKContext::PickPhysicalDevice()`
- **`OIVLib/Renderers/OIVVKRenderer/Source/VKContext.cpp`**
  - After device selection, always print a clear line identifying what was selected and why.
  - Scenarios:
    - **Default (fGpuIndex == -1)**: `Selected GPU: NVIDIA GeForce GTX 1650 (discrete GPU preferred)`
    - **Explicit index in range**: `Selected GPU: Intel UHD Graphics 630 (index 1, user requested)`
    - **Explicit index out of range**: `Warning: --gpu=99 out of range (2 GPUs available), falling back to NVIDIA GeForce GTX 1650 (discrete GPU)`
    - **Explicit index valid but not discrete when fallback would pick discrete**: still show what was actually selected
  - Include device name, index used, and selection reason.

### 2. Expose selected GPU index via IRenderer for F3 panel
- **`OIVLib/OIV/Include/Interfaces/IRenderer.h`**: Add `virtual int GetSelectedGPUIndex() const { return -1; }`
- **`OIVLib/Renderers/OIVVKRenderer/Source/VKContext.h`**: Add `int GetGpuIndex() const { return fGpuIndex; }`
- **`OIVLib/Renderers/OIVVKRenderer/Source/OIVVKRenderer.cpp`**: Implement `GetSelectedGPUIndex()` returning context's gpuIndex
- **`Clients/OIViewer/Source/ViewerApplicationCommands.cpp`**: In `CMD_ShowSystemInfo()`, show `GPU Index: 0` / `GPU Index: Auto` / `GPU Index: unknown`

### 3. Warn on out-of-range `--gpu` index in VKContext
- Already covered by change #1 above.

## Validation
1. `./bin/OIViewer --renderer=vulkan` → stdout shows selected GPU and reason (discrete GPU preferred)
2. `./bin/OIViewer --renderer=vulkan --gpu=0` → stdout shows selected GPU and reason (user requested index 0)
3. `./bin/OIViewer --renderer=vulkan --gpu=99` → stdout warning + fallback message
4. `./bin/OIViewer --renderer=vulkan --gpu=1` → stdout shows Intel selected, then error 4 from compositor
5. F3 panel shows GPU Index line matching the actual selection

## Notes
- `--gpu=1` (Intel) on Wayland cannot be made to work without fixing the driver/compositor explicit-sync protocol. The NVIDIA-only env-var gating is the correct workaround.
- If the user wants Intel to work, they would need to run under X11 instead of Wayland, or use a compositor that handles Intel's explicit sync correctly.
