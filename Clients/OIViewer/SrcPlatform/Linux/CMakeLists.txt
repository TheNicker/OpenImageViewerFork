target_compile_definitions(OIViewer PRIVATE OIVIEWER_PLATFORM_LINUX)

target_sources(OIViewer PRIVATE
    ApplicationLog.cpp
    ClipboardSetup.cpp
    ContextMenuBackend.cpp
    EntryPoint.cpp
    EventSync.cpp
    ExceptionHandler.cpp
    ExceptionHandler.h
    ImageControl.cpp
    ImageList.cpp
    MainWindow.cpp
    MonitorProvider.cpp
    PlatformFileDialog.cpp
    ShellCommandHandler.cpp
    ViewerApplication.cpp
    ViewerApplicationImage.cpp
    ViewerApplicationInput.cpp
    ViewerApplicationPlatformState.h
    ViewerApplicationUI.cpp
)
