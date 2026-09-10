# Command-line integration

OIViewer uses CLI11 v2.7.2, pinned to cbd58a3696887b34c70949aef21a71735a0c2ad5 in External/CLI11. This adapts the earlier integration plan to the implemented Vulkan renderer and current LWS lifecycle.

The passive client CommandLineParameters contains an optional native input path and shared RendererOptions. CLI11 stays private to CommandLine.cpp. Native Windows wide argv and Linux argv are parsed directly. Input tokens are joined with one space for compatibility; use -- before an option-like filename. Help/version/errors are returned as values, never process exits inside the parser.

--renderer accepts GL (OpenGL alias), D3D11 and Vulkan, case-insensitively. Unavailable backends fail before LWS initialization. Omitting it preserves Windows D3D11 preference and Linux Vulkan preference with automatic GL fallback when available. Explicit graphics selection suppresses fallback and Windows existing-instance forwarding.

--adapter selects the first suitable adapter with an exact, case-insensitive name. --adapter_index selects its zero-based backend enumeration index. They are mutually exclusive. Indices belong to the selected API and are not portable across APIs or machines. D3D11 and Vulkan support selection; GL reports unsupported instead of ignoring it. Missing, unusable and out-of-range explicit selections fail. --gpu is replaced by --adapter_index.

Options flow through ViewerApplication::Init, the platform native-handle adapter, OivRenderGateway, the synchronous initialization command and IPictureRenderer::Init. The command borrows its options pointer only during synchronous execution; no process-global preferences or extra per-frame state are retained. OIV validates again for library callers.

Windows file forwarding remains native to the Win32 entry point. A path-only invocation can forward to a tray instance; any explicit graphics option starts a new instance. Only terminal output attaches to the parent console. Console output uses UTF-16, redirected output stays UTF-8, and no console or message box is allocated for parsing diagnostics.

The system information shortcut is Shift+Tilde (stored as Shift+Grave to match the existing physical key naming). Plain Grave still toggles image information. Each system field has its own labeled row; missing driver/API data is identified as not reported. Vulkan exposes only its factory through Include, following D3D11's public/private header layout.

Initialization optionally returns a captured exception through a caller-owned exception_ptr. This is borrowed only for the synchronous command, like the options pointer. Expected selection failures use exceptions with owned messages so CLI diagnostics survive the existing result-code dispatcher; callers that omit the diagnostic pointer retain the previous result-code behavior. No error state is retained in the renderer after initialization.
