# Renderer build and startup policy

OIViewer uses CLI11 v2.7.2, pinned to `cbd58a3696887b34c70949aef21a71735a0c2ad5` in `External/CLI11`. The parser accepts native Windows wide arguments and Linux UTF-8 arguments. Help, version, and parsing errors return values to the entry point; they never initialize graphics.

## Build inclusion and default API

| Target | Enabled by default | Optional |
|---|---|---|
| Windows | Vulkan, D3D11 | GL |
| Linux | Vulkan, GL | None |

The switches are `OIV_BUILD_RENDERER_VK`, `OIV_BUILD_RENDERER_D3D11`, and `OIV_BUILD_RENDERER_GL`. Enabled renderers require their dependencies; configuration fails with the relevant disabling option if a dependency is missing. CMake never silently disables a renderer. D3D11 requires a Windows target. Viewer builds require at least one real renderer; internal Null rendering remains available for tests when `OIV_BUILD_TESTS` is enabled.

Vulkan requires SDK headers and a shader compiler that runs on the build host. Set `VULKAN_SDK`, or explicitly set `OIV_VULKAN_INCLUDE_DIR` and `OIV_GLSLC`. Linux also requires target Vulkan libraries. Windows loads `vulkan-1.dll` lazily at first Vulkan discovery/initialization, so the viewer and help/version can run without it. Missing or incomplete Windows runtimes become initialization diagnostics and permit eligible fallback. The private loader retains device-independent export trampolines for process lifetime, without per-frame loading or symbol resolution. Linux retains its linked runtime dependency.

Build inclusion, API preference, and acceleration are separate facts. CMake's compilation order does not choose a renderer. One private constant registry supplies compiled API order, canonical names, fixed adapter-selection capabilities, and factories. CLI metadata is derived from the same entries. Windows and Linux share the order `Vulkan -> D3D11 -> GL`, omitting implementations that were not built. The **default API** is the first compiled entry; determining it never probes the machine. A successful automatic selection can use a different API.

## Runtime selection

Explicit user constraints apply first. Within the eligible set, startup exhausts these groups in order:

| Acceleration | Windows API order | Linux API order |
|---|---|---|
| Hardware | Vulkan -> D3D11 -> GL | Vulkan -> GL |
| Unknown | Same compiled API order | Same compiled API order |
| Software | Vulkan -> D3D11 (including WARP) -> GL | Vulkan -> GL |

Acceleration describes the actual device/context. Vulkan uses physical-device type; D3D11 uses DXGI software flags or explicit WARP creation. GL recognizes software renderer names before known hardware-driver identities. Unrecognized GL vendors and translation layers are **Unknown**, never assumed to be hardware. Unknown Vulkan device types are also considered in the middle group.

Startup discovers APIs lazily and stops after success. Without a selector, existing automatic device preferences apply first (notably discrete Vulkan devices), followed by remaining eligible devices within the same tier. Capability and initialization failure on one adapter does not exclude other eligible adapters. A successful software Vulkan device cannot prevent trying hardware D3D11 or GL.

GL cannot classify its context before creating it. If provisional initialization reveals a lower acceleration tier, that instance is released before a competing candidate is attempted. GL is recreated only if selection later reaches its actual tier. An explicitly selected or sole GL context can be committed immediately without repeating initialization. Failed instances and all their resources are also released before the next attempt.

The final renderer receives application images only after successful initialization. Image resources release before the renderer, which releases before its native window. Fallback ends at startup: resize, surface recovery, and device errors preserve the existing renderer and never switch APIs mid-session. Candidate failures and deferrals use startup diagnostics; successful fallback opens no error dialog. Exhaustion reports the attempted APIs/adapters and failure reasons.

## Command-line constraints

`--renderer` accepts only the compiled canonical names `GL`, `D3D11`, and `Vulkan`, case-insensitively. `OpenGL` is not an alias. An explicit renderer prohibits cross-API fallback, while still allowing its eligible hardware, unknown, and software devices.

```text
OIViewer --renderer=Vulkan image.png
OIViewer --adapter_name=nViDiA folder
OIViewer --adapter_name="RTX 2000" image.png
OIViewer --adapter_index=0 image.png
OIViewer --renderer=D3D11 --adapter_name=Nvidia --adapter_index=1 image.png
```

`--adapter_name` replaces `--adapter`; the old spelling is rejected. Without an index, outer whitespace is trimmed and an empty effective query is rejected. Nvidia, AMD, and Intel match manufacturer IDs, so `AMD` can match a device displayed only as Radeon. Other queries match GPU-name substrings. Comparisons fold ASCII letters without locale conversion, preserving UTF-8 bytes. D3D11 names are explicitly encoded from native UTF-16 as UTF-8; system information explicitly decodes backend text back to native text. Neither conversion uses the C locale. There is no regex, fuzzy scoring, or extra GPU ranking. The first usable match in enumeration order within the applicable tier wins; a matching device that fails capabilities or initialization does not stop the search. The name constraint survives API fallback.

`--adapter_index` accepts a nonnegative integer; zero is explicit. It overrides the name when both are present. It binds to the explicit renderer, or otherwise to the build's default API, and selects that API's indexed adapter only. There is no fallback to another adapter or API. Negative, malformed, overflowing, out-of-range, or unusable indices fail; a matching name cannot rescue an invalid index. API indices are not portable between APIs or machines.

GL cannot explicitly choose an adapter. Automatic selection skips GL when an effective adapter constraint exists. Explicit GL with such a constraint fails, as does an index bound to GL in a GL-only build.

| Request or machine | Result |
|---|---|
| Hardware Vulkan initializes | Vulkan; later APIs are not discovered |
| Software Vulkan, hardware D3D11 | D3D11 |
| Software Vulkan, hardware GL | GL |
| Confirmed hardware fails, GL is unknown | Unknown GL before software |
| Explicit Vulkan, all its hardware fails | Try remaining eligible Vulkan tiers only |
| Several matching AMD adapters; first fails | Next usable AMD match in that tier |
| Name requests Nvidia, index selects Intel | Indexed Intel device only |
| Invalid index with a valid name | Fail |
| Missing Windows Vulkan runtime, automatic selection | Try eligible alternatives |
| Same runtime failure with explicit Vulkan or index bound to Vulkan | Fail |

Help lists exactly the built choices and their order within the acceleration policy. The constant registry is read without loading a graphics runtime. Input tokens are joined with spaces for compatibility; use `--` before a filename that looks like an option. Windows paths stay native, console output uses UTF-16, and redirected diagnostics use UTF-8.

A path-only Windows invocation can forward to an existing tray instance. Any explicit renderer, name, or index option starts a new instance so the requested graphics configuration is honored.

## System information and library boundary

Shift+Grave toggles system information. Each field uses the shared row formatter, including **Adapter index** immediately after **Adapter**, followed by **Acceleration**. Unavailable indices display **Not reported**; acceleration displays **Hardware**, **Software**, or **Unknown**. Details describe the selected renderer and device.

`RendererOptions` flows through `ViewerApplication::Init`, the native-handle adapter, `OivRenderGateway`, the synchronous initialization command, and `IPictureRenderer::Init`. OIV also validates library callers. Borrowed options and `exception_ptr` storage live through the synchronous command; diagnostics own their messages. CLI11 remains private to the client parser, and backend implementation headers remain private to their factories.

## Validation

Use Debug/Ninja in `build/windows-clang` (clang-cl with the Visual Studio environment) and `build/linux-clang` (Clang through WSL). Reconfigure caches when flags or revisions change. Git metadata is computed at configuration; when WSL cannot read the Windows worktree metadata, pass freshly obtained host values through `OIV_VERSION_REVISION` and `OIV_GIT_SHORT_HASH`. Build-matrix configurations run sequentially in those directories and restore defaults afterward.

Tests cover candidate tier/API ordering, lazy discovery, adapter matching and precedence, failed/provisional lifetime, exact CLI choices, system-info formatting, Vulkan initialization failures, fixed queue/descriptor layouts, and isolated Windows runtime-loader failure. Shader layout assertions check the C++ side alongside shader compilation and available rendering tests. See `RendererPolicyValidation.md` for executed checks and environmental gaps.
