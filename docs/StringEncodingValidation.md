# String encoding validation

Validation on 2026-09-10 used Windows clang-cl 22.1.8 and Linux Clang 21.1.7 through WSL. Application builds use
Debug/Ninja in `build/windows-clang` and `build/linux-clang`. Standalone performance measurements use optimized
builds (`/O2` and `-O3`), not Debug timings.

## Correctness and integration

| Suite | Windows | Linux |
|---|---|---|
| Main unit/integration executable | 149 cases passed | 152 cases passed |
| Separate font executable | 1 case passed | 1 case passed |
| Explicit clipboard integration | 1 case passed | Windows-specific |
| UTF-8 active-code-page font process | 2 cases passed | Windows-specific |
| Vulkan failure doubles | 29 cases passed | 29 cases passed |
| Missing Vulkan runtime / D3D11 fallback | 3 cases passed | Windows-specific |

- Focused tests cover every supported conversion direction/input form, literals, embedded NULs, null pointers,
  scalar boundaries, malformed Unicode, byte-preserving copies, moved storage, and locale independence.
- Bounded-copy tests cover zero/one/exact/truncated capacities, UTF-8 and UTF-16 boundaries, overlap, termination,
  no padding, and nonterminated source views. Inputs satisfy the valid-Unicode, NUL-free contract.
- Extension cases cover directories, dotfiles, roots, and native separators. A separate oracle matched 1,512
  ordinary lexical paths against std::filesystem on each platform; alternate-data-stream syntax is outside scope. Direct splitting matches the previous
  stream behavior, including trailing-NUL removal and originally nonempty all-NUL tokens. Trimming retains coverage.
- A Python Unicode oracle supplied all 1,112,064 scalar values, including NUL, as UTF-8/UTF-16/UTF-32. All applicable
  conversion directions matched on both platforms. Every two-byte combination (65,536 inputs) was checked against
  independently classified valid UTF-8. Linux ran this probe with address and undefined-behavior sanitizers.
- Standalone narrow and wide Windows font-boundary configurations also passed syntax compilation.
- Both compilers accept the C++23 feature probe and emit the intended errors when native `char8_t` is disabled or
  `resize_and_overwrite` is unavailable. Unsupported conversion types are rejected through compile-time constraints.
- The Windows clipboard integration test passed Unicode round-trips, malformed-input failure without replacing the
  prior text, synthesized CF_TEXT for a legacy consumer, and DIBV5 preference over text. The explicit test harness
  snapshots and restores the desktop clipboard. The test is hidden from the default suite.
- Windows error-message tests cover wide/UTF-8/char8_t consistency, no error, and unavailable-message behavior.
- Font tests cover normal paths and rejection of paths unrepresentable by the Windows file API code page. A separate
  process with a UTF-8 active-code-page manifest also loaded the Unicode font path. Linux loaded UTF-8 and raw-byte
  filenames. Font checks run in a separate executable to avoid the system PNG dependency colliding with ImageCodec's
  bundled APNG implementation inside the codec test process.
- Compiler command inspection confirmed that OIV, LWS, ImageCodec, and FreeTypeWrapper use the top-level LLUtils
  include directory first. LInput headers are consumed through viewer compilation; there is no standalone LInput
  compilation target in these configurations. Modified dependencies pin the same LLUtils revision.

## Performance

The standalone [benchmark](../Tests/Benchmarks/StringUtilityBenchmark.cpp) compares the original locale-based
conversion, strncpy/wcsncpy copying, and stream splitting against the final implementation. Conversions use
identical valid text under a UTF-8 locale; correctness differences on malformed data are not timed. Seven batches
rotate implementation order, consume the results, and report median nanoseconds per call. Copy routines have
intentionally different guarantees: the new function requires a valid, NUL-free view and appends a terminator,
while the original scans/pads and may leave truncated output unterminated.

These are local helper measurements, not viewer frame-rate claims. Sub-nanosecond differences in the small Linux
copy cases are within observed run-to-run variation. Large copies and conversion/splitting show clear improvements;
no repeatable regression remained in the sampled workloads.

| Operation | Windows old → new (ns) | Linux old → new (ns) |
|---|---:|---:|
| ASCII UTF8 to wide (32 Windows / 32 Linux input units) | 256.2 → 52.1 | 61.8 → 28.8 |
| ASCII wide to UTF8 (32 Windows / 32 Linux input units) | 164.8 → 52.8 | 71.8 → 17.1 |
| ASCII UTF8 to wide (4096 Windows / 4096 Linux input units) | 23001.7 → 1645.2 | 3939.0 → 1843.6 |
| ASCII wide to UTF8 (4096 Windows / 4096 Linux input units) | 10563.7 → 1710.0 | 4634.3 → 1244.0 |
| Unicode UTF8 to wide (36 Windows / 36 Linux input units) | 221.9 → 49.8 | 97.4 → 21.1 |
| Unicode wide to UTF8 (20 Windows / 16 Linux input units) | 167.4 → 51.5 | 76.6 → 22.4 |
| Unicode UTF8 to wide (4608 Windows / 4608 Linux input units) | 16749.6 → 2367.2 | 6772.6 → 2892.6 |
| Unicode wide to UTF8 (2560 Windows / 2048 Linux input units) | 14625.0 → 3242.9 | 7121.6 → 2816.9 |
| copy ASCII char 32 | 5.0 → 3.7 | 3.4 → 3.2 |
| copy ASCII wchar 32 | 8.6 → 3.9 | 6.3 → 4.2 |
| copy ASCII char 4096 | 197.5 → 30.8 | 49.5 → 26.6 |
| copy ASCII wchar 4096 | 179.9 → 59.3 | 68.3 → 69.7 |
| copy truncated UTF8 | 8.8 → 4.6 | 3.8 → 3.9 |
| copy truncated wide | 11.8 → 4.9 | 5.0 → 3.9 |
| split 64/80/72 | 1069.9 → 179.9 | 145.8 → 42.1 |
| extension short | 3.2 → 2.3 | 1.8 → 1.8 |
| extension long basename | 2.1 → 2.1 | 1.7 → 1.7 |
| extension nested path | 2.1 → 2.1 | 1.7 → 1.7 |
| lower short | 230.9 → 21.6 | 35.0 → 7.9 |
| lower long | 33165.8 → 146.6 | 3044.2 → 153.7 |

Extension rows use a separate isolated run with one million calls per batch, avoiding background-build noise
in these very short operations. The backward suffix scan replaced a provisional implementation that took about
69 ns for a long basename on Linux; the final implementation matched the original routine at about 1.7 ns.

Heap-sized transcoding samples dropped from two allocations to one. Copying and same-type moves allocate nothing.
Splitting the three long tokens used 13 → 6 total allocation calls on Windows and 9 → 6 on Linux, including vector
and stream implementation allocations. The final split emits three owning tokens without an intermediate stream.

The one-pass converter was also faster than the provisional two-pass converter in all eight measured conversion
cases on each platform. The earlier validating StrCpy implementation was discarded after its per-code-point scan
showed substantial overhead.

Spare capacity is intentional. For 4,096 ASCII wide code units, UTF-8 output retained capacity 12,303 on Windows and
16,384 on Linux, including library rounding. This avoids sizing scans, preliminary zero-fill, and shrinking work.

## Runtime checks and limits

Windows Vulkan and D3D11 passed empty/image/folder startup, minimize/restore, and normal shutdown. D3D11 software
fallback also passed. A D3D11 image path containing an accent, Hebrew, and a supplementary character was preserved
in the window title and closed normally.

Linux Vulkan passed empty/image/folder startup with confirmed Wayland renderer-buffer presentation. A Unicode image
path also started and presented under `LC_ALL=C`. These Linux probes observed process liveness and presentation,
then terminated their own processes; they did not verify normal GUI shutdown or visually inspect image pixels.
Malformed incoming clipboard buffer handling was reviewed at the platform boundary, rather than injected through
the viewer UI. Windows font tests covered the machine's default file code page and a process-local UTF-8 code page,
not every legacy code page. A whole-application sanitizer build and remote CI were not run.

## Reproduction

Build and run the regular suites, including the separate font executable:

```powershell
./build/windows-clang/run.ps1
./build/windows-clang/bin/tests.exe --reporter compact
./build/windows-clang/bin/tests_font_encoding.exe --reporter compact
./build/windows-clang/bin/tests_vk_failures.exe --reporter compact
./build/windows-clang/bin/tests_vk_runtime_failure.exe --reporter compact
```

```sh
cmake --build build/linux-clang --parallel 12
build/linux-clang/bin/tests --reporter compact
build/linux-clang/bin/tests_font_encoding --reporter compact
build/linux-clang/bin/tests_vk_failures --reporter compact
```

Run the benchmark from a Visual Studio development shell with clang-cl on PATH, or a Linux shell:

```powershell
clang-cl /nologo /std:c++23preview /O2 /EHsc /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /IExternal/LLUtils/Include Tests/Benchmarks/StringUtilityBenchmark.cpp /Febuild/windows-clang/string-benchmark.exe /Fobuild/windows-clang/string-benchmark.obj
./build/windows-clang/string-benchmark.exe
```

```sh
clang++ -std=c++23 -O3 -IExternal/LLUtils/Include Tests/Benchmarks/StringUtilityBenchmark.cpp -o build/linux-clang/string-benchmark
build/linux-clang/string-benchmark
```

The clipboard test is explicitly selected with `[clipboard]`; preserve/restore clipboard data when running it.
Local oracle, feature-probe, clipboard-restoration, active-code-page, and Unicode-startup harnesses are in the ignored
`build/string-copy-review` directory. Benchmark timings are observations, not timing thresholds in unit tests.
