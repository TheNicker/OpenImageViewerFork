# Open Image Viewer

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/22d2c9bc0fa149fcaf0b84e009839fa9)](https://app.codacy.com/gh/OpenImageViewer/OpenImageViewer)
[![Windows build](https://github.com/OpenImageViewer/OpenImageViewer/actions/workflows/build-windows.yaml/badge.svg)](https://github.com/OpenImageViewer/OpenImageViewer/actions/workflows/build-windows.yaml)

**Open Image Viewer (OIViewer)** is a hardware-accelerated, open-source C++26 image viewer focused on accurate image presentation, fast navigation, and keyboard-driven workflows.

It aims to present images accurately instead of simply displaying image data through the monitor color space.

OIViewer embraces modern operating systems, graphics APIs, and C++ features. Simplicity and performance guide its design. OIViewer adopts the latest technologies that are generally available on its CI servers.

[Website](https://www.openimageviewer.com) · [Features](https://www.openimageviewer.com/#features) · [A word from the author](https://www.openimageviewer.com/#word)

![Selection demonstration](https://i.ibb.co/NZXpb2W/cut.gif)

## Features

- Cross-platform support for Windows and Linux.
- Vulkan, D3D11, and OpenGL rendering with hardware-first startup selection.
- Fast folder browsing, sorting, slideshows, zooming, panning, and fullscreen viewing.
- Image information, texel grids, pixel inspection, and selection tools.
- Cropping, clipboard operations, rotation, flipping, and color correction.
- Keyboard shortcuts, with the active bindings available through **F1**.
- Consistent high-DPI scaling with pixel-accurate image zoom.

## Runtime requirements

Release packages target 64-bit Windows and Linux x86_64. A graphics driver supporting Vulkan 1.1+, Direct3D 11, or OpenGL 3.0+ is required, depending on the renderers included in the build. When moving or copying OIViewer, keep the entire extracted package folder intact so the executable can find its libraries and resources.

### Windows

Windows 7 SP1, 8, 8.1, 10, and 11 are supported targets. **Windows 11 or newer is recommended for the best user experience.**

When using Windows 7 SP1, install:

* [KB2670838 - Windows 7 platform update](https://www.microsoft.com/en-us/download/details.aspx?id=36805)
* [KB4019990 - D3DCompiler_47](https://www.catalog.update.microsoft.com/Search.aspx?q=4019990)
* [Universal C runtime](https://support.microsoft.com/en-us/topic/update-for-universal-c-runtime-in-windows-c0514201-7fe6-95a3-b0a5-287930f3560c)

### Linux

Official Linux binaries require **glibc 2.39 or newer**. Private builds are not guaranteed to satisfy this requirement. A Wayland desktop is required.

## Build from source

### Shared prerequisites

- Git
- CMake 3.24 or newer, with C++26 support for the selected toolchain
- A C++26-capable compiler and compatible standard library. **Clang/clang-cl 21 or newer is required when using Clang.**

Recommended development stack: **CMake, Ninja, Clang (clang-cl on Windows), and VS Code**.

### Clone

Image codecs and many other dependencies are built from source through repository submodules. Clone them together with the project:

```sh
git clone --recursive https://github.com/OpenImageViewer/OpenImageViewer.git
cd OpenImageViewer
```

If the repository was cloned without submodules, initialize them before configuring:

```sh
git submodule update --init --recursive
```

### Building on Windows

**The Windows SDK must be installed.** MSVC and Visual Studio 2026 or newer are also supported. The recommended stack above remains preferred.

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl
cmake --build build --parallel
```

For a Visual Studio generator, configure without `-G Ninja` and the Clang compiler options, then build with `cmake --build build --config Release`.

CMake can download the Vulkan build tools when needed. Install 7-Zip for SDK extraction, or provide an existing Vulkan SDK.

### Building on Linux

Install the development packages for GTK 3, Wayland, OpenGL/EGL, and Vulkan, together with `pkg-config` and the Wayland protocol tools.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build --parallel
```

Both builds place the application and resources in `build/bin`. See the [CI workflows](.github/workflows) for current toolchain and dependency setup.

## Packaging

For a release-style package, run:

```powershell
.\publish.ps1
```

The publish script uses Ninja internally and requires 7-Zip when packaging is enabled. On Linux, invoke it as `pwsh ./publish.ps1`. The script verifies that `7z` is available before configuring and produces a Linux `.7z` runtime package after building OIViewer.

## Command line

```text
OIViewer "path/to/image-or-folder"
OIViewer --help
```

Use `--help` for available options, including renderer and GPU selection.

## License

OIViewer is distributed under the [OpenImageViewer License](LICENSE.md).
