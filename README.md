# Open Image Viewer

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/22d2c9bc0fa149fcaf0b84e009839fa9)](https://app.codacy.com/gh/OpenImageViewer/OpenImageViewer?utm_source=github.com&utm_medium=referral&utm_content=OpenImageViewer/OpenImageViewer&utm_campaign=Badge_Grade_Dashboard)
[![Windows MSVC build](https://github.com/OpenImageViewer/OpenImageViewer/actions/workflows/build-windows.yaml/badge.svg)](https://github.com/OpenImageViewer/OpenImageViewer/actions/workflows/build-windows.yaml)

**Open Image Viewer** is a hardware-accelerated, open-source C++26 image viewer focused on accurate image presentation, fast navigation, and efficient keyboard-driven workflows.

It aims to present images accurately instead of simply displaying image data through the monitor color space.

For more information visit [www.openimageviewer.com](https://www.openimageviewer.com).

[A Word from the author](http://www.openimageviewer.com/#word)  
[Highlights and features](http://www.openimageviewer.com/#features)

![Selection rect demonstration with Open Image Viewer](https://i.ibb.co/NZXpb2W/cut.gif "Preview")

## Features

* Rendering with Vulkan, D3D11, and OpenGL, with hardware-first startup selection.
* Fast folder browsing, sorting, slideshow playback, zooming, panning, and fullscreen viewing.
* Keyboard-first operation with the active key bindings available from F1.
* Image inspection tools including image information, texel grid, pixel inspection, and selection workflows.
* Common image actions such as crop, copy selection, paste image, rotation, flipping, and color correction.
* Codecs and third-party dependencies are built from source via repository submodules.

## Supported Platforms

Windows is the supported viewer target. 64-bit builds are the official release path. 32-bit builds may compile and run but are not part of the official release flow.

The Linux Wayland viewer is available as an experimental, unofficial target. Core library and test builds may be
configured with `-DOIV_BUILD_CLIENT=OFF` when a viewer executable is not required.

### Window and rendering coordinates

OIViewer configures the same logical client area on every window-system backend. Its initial `946 x 602` client size
uses 96-DPI logical units and is calibrated to approximate the historical `1200 x 800` native Win32 window at 125%
scaling. The complete outer size remains an operating-system decision and can vary with DPI, theme, and decoration
policy. The image sidebar reserves 160 logical layout units, corresponding to 200 native pixels at the 125% reference;
Win32 privately subtracts its native scrollbar width from the child client area so the complete sidebar still occupies
that allocation. Its font, row, and displayed-thumbnail dimensions use the same reference calibration so DPI scaling
does not enlarge the historical sidebar presentation.

Window layout and input enter through LWS logical coordinates. Rendering uses the exact framebuffer size reported in
the same `ClientAreaSize` snapshot, and OIViewer converts pointer positions to that framebuffer coordinate space before
applying image transforms. This keeps the renderer, zoom/pan calculations, and high-DPI presentation consistent
without a platform-specific outer-window sizing API.

On Wayland, logical units are compositor surface coordinates rather than physical-monitor DPI. OIViewer uses
`wp_fractional_scale_v1` with `wp_viewporter` when available and otherwise uses the entered outputs' integer scale.
Scale-matched buffers keep 100% image zoom pixel-accurate; the compositor controls top-level placement and may adjust
the requested logical size. OIViewer sets its Wayland app ID, while compositor window icons come from the matching
installed desktop-file metadata rather than an icon attached to the window.

### Text encoding

Text uses UTF-8 on Linux and UTF-16 at wide Windows interfaces. String conversion is locale-independent;
see the [StringUtility contracts](External/LLUtils/Include/LLUtils/StringUtility.h) and [encoding checks](Tests/Source/TestUtf8.cpp).

### Windows Runtime Notes

Windows 7 SP1, 8, 8.1, 10, and 11 are supported targets.

When using Windows 7 SP1, install:

* [KB2670838 - Windows 7 platform update](https://www.microsoft.com/en-us/download/details.aspx?id=36805)
* [KB4019990 - D3DCompiler_47](https://www.catalog.update.microsoft.com/Search.aspx?q=4019990)
* [Universal C runtime](https://support.microsoft.com/en-us/topic/update-for-universal-c-runtime-in-windows-c0514201-7fe6-95a3-b0a5-287930f3560c)

## Build From Source

### Prerequisites

* Git
* CMake 3.24 or newer
* Windows SDK
* One supported Windows build setup:
  * Ninja with a C++26-capable clang/clang-cl or MSVC toolchain
  * Visual Studio Build Tools 2026 or newer with MSVC

Recommended Windows development stack: CMake, Ninja, clang-cl, VS Code, and the Windows SDK. This stack does not require Visual Studio Build Tools or MSVC.

### Clone

```powershell
git clone --recursive --depth 1 https://github.com/OpenImageViewer/OpenImageViewer.git
cd OpenImageViewer
```

If the repository was cloned without submodules, initialize them before configuring:

```powershell
git submodule update --init --recursive
```

### Configure and Build

Run one of the following from an environment where the selected compiler and Windows SDK are available.

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Or use the Visual Studio generator:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The viewer executable and copied resources are generated under the build tree's `bin` directory.

## Packaging

For a release-style package, run:

```powershell
.\publish.ps1
```

The publish script uses Ninja internally and requires 7-Zip when packaging is enabled.
On Linux, invoke it as `pwsh ./publish.ps1`. The script verifies that `7z` is available before
configuring and produces a Linux `.7z` runtime package after building OIViewer.

## Tests

Tests are built by default with the main CMake configuration.

```powershell
cmake --build build --target tests
build\bin\tests.exe
```

## License

OIV is distributed under the [OpenImageViewer License](LICENSE.md).

## Command line

```text
OIViewer [--renderer=GL|D3D11|Vulkan] [--adapter_name="vendor or GPU text"] [--adapter_index=n] [--] [image or folder]
OIViewer --renderer=Vulkan --adapter_index=1 "photos/cat.jpg"
OIViewer --renderer=D3D11 --adapter_name=Intel
OIViewer --help
OIViewer --version
```

Renderer names are case-insensitive. Help lists only compiled backends; the old OpenGL alias is rejected. An explicit renderer restricts startup to that API, including its eligible software devices.

Adapter selection is supported by D3D11 and Vulkan. Names match vendor IDs or GPU-name substrings ignoring ASCII case. Indices are zero-based and specific to the selected API; an index overrides a supplied name and disables fallback. GL uses the platform-selected adapter and rejects explicit adapter options. Deprecated --adapter and --gpu options are rejected.

Quoted Unicode paths are preserved. Unquoted positional tokens are joined with spaces; use -- before a path beginning with a dash. On Windows, explicit graphics options start a new instance rather than forwarding to an existing tray instance. Windows terminal launches use the existing console or redirected streams, and the shell waits until the process exits. Desktop viewer launches detach a console created solely for them, which may briefly appear.

Press **Shift+Tilde** for system information; plain Tilde retains image information.

## Renderer selection

Windows builds enable Vulkan and D3D11 by default; GL is optional. Linux builds enable Vulkan and GL. Enabled backends require their development dependencies, and CMake reports missing dependencies instead of disabling a renderer silently.

Automatic startup prefers hardware, then unknown acceleration, then software. API order within each group is Vulkan, D3D11 (Windows), then GL, skipping backends that were not built. `--renderer` fixes the API. `--adapter_name` matches vendor/name text; `--adapter_index` overrides the name, uses the selected or default built API, and disables fallback. GL cannot explicitly select an adapter. Use `--help` for the choices in your build.

Developer contracts live beside [renderer options](OIVLib/OIV/Include/Interfaces/RendererOptions.h) and [startup selection](OIVLib/OIV/Source/RendererSelection.h). Use `OIViewer --help` for available choices and examples.
