# Renderer policy validation

Validated on 2026-09-10 using Debug and Ninja in the current worktree. Windows used clang-cl 22.1.8 with the Visual Studio development environment; Linux used Clang 21.1.7 through WSL. Both stable build directories were restored to the default renderer sets after matrix checks.

## Builds and automated tests

| Check | Result |
|---|---|
| Windows default Vulkan + D3D11 | Build passed; 140 unit test cases passed |
| Linux default Vulkan + GL | Build passed; 144 unit test cases passed |
| Vulkan failure doubles, both platforms | 29 cases passed on each platform |
| Windows missing-runtime fixture | 3 cases passed, including real D3D11 initialization |
| Windows Vulkan-only, D3D11-only, GL-only, all three | Builds and compiled-choice/help/policy tests passed |
| Linux Vulkan-only, GL-only | Builds and compiled-choice/help/policy tests passed |
| Test-only Null, no real renderers, viewer disabled | Windows build and policy/CLI tests passed |
| Viewer with no real renderer | Configuration correctly rejected |
| D3D11 on Linux | Configuration correctly rejected with its disabling option |
| Missing Vulkan headers, missing GL dependencies | Configuration correctly rejected with the relevant option |
| Unrunnable host shader compiler | Configuration correctly rejected with the relevant option |
| GL-only viewer with an automatic adapter-name constraint | Exited with a specific unsupported-selection diagnostic before window initialization |
| Windows executable imports | No mandatory Vulkan DLL import |
| Linux executable dependencies | Linked `libvulkan.so.1` retained |
| Workflow YAML and changed-file checks | Parsed successfully; `git diff --check` passed; authored files use CRLF |

The controlled selection tests exercise hardware over software across APIs, Unknown before Software, explicit API restrictions, failed/provisional resource release before another factory runs, lazy discovery, vendor-ID and substring matching, index precedence, unsupported/no-match candidates, automatic GPU preferences, and diagnostic exhaustion. Single-context GL initialization is also checked to avoid repeated initialization when there is no competing device. A Unicode regression test forces the C locale and checks native/UTF-8 conversion of accented text, Hebrew, and a supplementary character, plus malformed Windows input.

Vulkan failure doubles check shared and distinct graphics/presentation queue families in both index orders, extension requests, zero/one descriptor bindings, initialization failures, and resource cleanup. Compile-time assertions cover registry structure and shader host layouts. The missing-runtime executable compiles the production Windows loader against an absent module name; it does not modify installed DLLs. It checks help/version, repeated loading failure, explicit Vulkan and default-API index failure, and automatic D3D11 fallback.

## Runtime checks

| Platform / backend | Observed result |
|---|---|
| Windows Vulkan, D3D11, GL | Empty, image, and folder startup passed; minimize/restore and normal exit 0 passed |
| Windows D3D11 WARP | Image startup, minimize/restore, and normal exit 0 passed; Software and the reported nonzero adapter index were displayed |
| Windows Unicode image path | GL image startup preserved the Hebrew/accented filename in the window title; minimize/restore and normal exit passed |
| Linux automatic image/folder startup | Software GL was deferred; software Vulkan selected and presented; minimize/restore and normal exit 0 passed |
| Linux explicit Vulkan and GL | Empty, image, and folder startup passed; minimize/restore and normal exit 0 passed |
| Linux forced software fallback | Selected software Vulkan after GL classification; presentation, minimize/restore, and normal exit passed |
| Linux automatic empty startup | Remained alive and presented a Vulkan buffer; the probe could not uniquely identify its native window and terminated its own process |

Windows hardware checks used an NVIDIA RTX 2000 Ada Generation; WARP used the Microsoft Basic Render Driver. Linux rendering used Mesa llvmpipe. Wayland traces confirmed Vulkan presentation-queue buffer attachments for every case selecting Vulkan. The Windows WARP screenshot was inspected: the image rendered, and Adapter index and Acceleration appeared as aligned, colored rows immediately after Adapter.

## Coverage limits

- No local MinGW toolchain was available. The cross-build workflow now installs host shader tools and stages target Vulkan headers, but remote CI and release-packaging jobs were not run.
- Physical AMD/Intel selection and mixed hardware/software ordering across different real GPUs were covered through controlled candidates rather than an available multi-vendor hardware matrix.
- The available Windows capture methods did not yield a verifiable Vulkan hardware image. Its startup, minimize/restore, and normal shutdown passed; Vulkan software presentation was observed on Linux.
- Native window automation could not uniquely select the Linux automatic empty-startup window, including a retry. Presentation and process liveness were verified for that case; normal shutdown was verified for explicit Vulkan/GL empty startup and the automatic image/folder cases.
- Hardware/Software/Unknown formatting and zero/nonzero/unreported indices are covered by formatter tests. The inspected live system-info screenshot covered Software with a nonzero index.

## Reproduction

From a Visual Studio development shell with LLVM and Vulkan build tools available:

```powershell
cmake -S . -B build/windows-clang -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DOIV_BUILD_RENDERER_VK=ON -DOIV_BUILD_RENDERER_D3D11=ON -DOIV_BUILD_RENDERER_GL=OFF
cmake --build build/windows-clang
./build/windows-clang/bin/tests.exe
./build/windows-clang/bin/tests_vk_failures.exe
./build/windows-clang/bin/tests_vk_runtime_failure.exe
```

From the worktree in WSL, with freshly obtained host Git metadata supplied if WSL cannot interpret the worktree's Git file:

```sh
cmake -S . -B build/linux-clang -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DOIV_BUILD_RENDERER_VK=ON -DOIV_BUILD_RENDERER_GL=ON -DOIV_BUILD_RENDERER_D3D11=OFF
cmake --build build/linux-clang
./build/linux-clang/bin/tests
./build/linux-clang/bin/tests_vk_failures
```

Local logs and smoke captures are under the corresponding stable build directories. These are validation artifacts and are not versioned.
