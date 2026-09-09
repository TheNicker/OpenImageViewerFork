# System Info Overlay Plan

## Goal

Add a system information overlay (F3) that displays runtime renderer/backend details and system info, matching the existing F1 key-bindings overlay style.

## Existing Pattern

- `F1` → `CMD_ToggleKeyBindings` → `LabelManager` text overlay with `IRM_Overlay`
- `MessageHelper::CreateKeyBindingsMessage()` builds a formatted string
- `MessageFormatter::FormatMetaText()` handles dot-leaders and color styling

## Tasks

### 1. Expose renderer identity at runtime

**Problem:** `IRenderer` has no `GetName()` / type query. `RendererName()` in `OIV.cpp` is `constexpr` and build-time only.

**Solution:**

- Add to `OIVLib/OIV/Include/Interfaces/IRenderer.h`:

  ```cpp
  virtual const char* GetBackendName() const = 0;
  virtual const char* GetGPUName() const { return ""; }
  virtual const char* GetAPIVersion() const { return ""; }
  virtual const char* GetDriverVersion() const { return ""; }
  ```

- Implement in each renderer:
  - `OIVGLRenderer` → `"OpenGL"`, `glGetString(GL_RENDERER)`, `glGetString(GL_VERSION)`, `""`
  - `OIVD3D11Renderer` → `"D3D11"`, DXGI adapter desc, `"11.0"`, driver version from `DXGI_ADAPTER_DESC`
  - `OIVVKRenderer` → `"Vulkan"`, `VkPhysicalDeviceProperties.deviceName`, `VkPhysicalDeviceProperties.apiVersion` formatted, `VkPhysicalDeviceProperties.driverVersion`
  - `NullRenderer` → `"Null"`

### 2. Gather OS / system info

- **Windows:** Use existing `LLUtils::PlatformUtility::GetOSVersion()` (works).
- **Linux:** `GetOSVersion()` throws. Add a small helper in `ViewerApplication` or `MessageHelper` to read `/etc/os-release` (`PRETTY_NAME`) or fall back to `uname -srmo`.
- **CPU cores:** Use existing `LLUtils::PlatformUtility::GetCPUCoresInfo()`.
- **App version:** Use `OIV::CurrentVersion` + `OIV_GIT_SHORT_HASH` + build type (`#ifdef NDEBUG`).

### 3. Add `MessageHelper::CreateSystemInfoMessage()`

In `OIVAppCore/Source/Src/MessageHelper.cpp` and header:

```cpp
static LLUtils::native_string_type CreateSystemInfoMessage();
```

Builds a `MessageFormatter::FormatArgs` table with rows:

- Application
  - Name: "OpenImageViewer"
  - Version: `FormatFullVersion(CurrentVersion)`
  - Build: "Release" / "Debug"
  - Git: `OIV_GIT_SHORT_HASH`
- Operating System
  - OS: Windows version string or Linux `PRETTY_NAME`
  - Kernel: `uname -srmo` (or empty on Windows)
  - CPU cores: physical / logical
- Renderer
  - Backend: `OIV::GetRenderer()->GetBackendName()`
  - GPU: `OIV::GetRenderer()->GetGPUName()`
  - API version: `OIV::GetRenderer()->GetAPIVersion()`
  - Driver: `OIV::GetRenderer()->GetDriverVersion()`

Style: header in magenta (`DefaultHeaderColor`), keys in orange, values in green, matching existing overlays.

### 4. Add command `CMD_ShowSystemInfo`

In `Clients/OIViewer/Source/ViewerApplicationCommands.cpp`:

- Register `"cmd_show_system_info"` callback
- Implementation mirrors `CMD_ToggleKeyBindings`:
  - Label name: `"systemInfo"`
  - Background: black with ~180 alpha
  - Font: fixed font, size 12
  - Position: `{20, 60}`
  - Text: `MessageHelper::CreateSystemInfoMessage()`
  - Render mode: `IRM_Overlay`

### 5. Bind F3 key
In `Clients/OIViewer/Resources/Configuration/KeyBindings.json`:
```json
{ "F3": "ShowSystemInfo" },
```

### 6. Verify Vulkan renderer at runtime
Once implemented, pressing `F3` will show `Backend: Vulkan` in the overlay. No separate verification step needed.

## Files to modify

| File | Change |
|------|--------|
| `OIVLib/OIV/Include/Interfaces/IRenderer.h` | Add 4 virtual methods |
| `OIVLib/Renderers/OIVGLRenderer/OIVGLRenderer.h/.cpp` | Implement new methods |
| `OIVLib/Renderers/OIVD3D11Renderer/Source/OIVD3D11Renderer.h/.cpp` | Implement new methods |
| `OIVLib/Renderers/OIVVKRenderer/Source/OIVVKRenderer.h/.cpp` | Implement new methods |
| `OIVLib/OIV/Source/NullRenderer.h/.cpp` | Implement new methods |
| `OIVAppCore/Source/Include/OIVAppCore/MessageHelper.h` | Declare `CreateSystemInfoMessage()` |
| `OIVAppCore/Source/Src/MessageHelper.cpp` | Implement `CreateSystemInfoMessage()` |
| `Clients/OIViewer/Source/ViewerApplicationCommands.cpp` | Add `CMD_ShowSystemInfo` |
| `Clients/OIViewer/Resources/Configuration/KeyBindings.json` | Add F3 binding |

## Open questions (resolved)
- **Key:** F3 (free, consistent with F1/F2).
- **GPU detail:** Name + API version + driver version.
- **Overlay style:** Same as F1 (in-window text overlay via `LabelManager`).
