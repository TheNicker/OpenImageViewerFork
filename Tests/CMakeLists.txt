
file(GLOB_RECURSE sourceFiles CONFIGURE_DEPENDS
    "./Source/*.h"
    "./Source/*.cpp"
)

set(ExternalFolder "${CMAKE_CURRENT_SOURCE_DIR}/../External")
cmake_path(ABSOLUTE_PATH ExternalFolder NORMALIZE OUTPUT_VARIABLE ExternalFolder)

set(TargetName tests)

if(NOT TARGET Catch2::Catch2WithMain)
    add_subdirectory(${ExternalFolder}/catch2 ./external/catch2)
endif()

add_executable (${TargetName} ${sourceFiles})
target_sources(${TargetName} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/Source/ImageList.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_sources(${TargetName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/SrcPlatform/Win32/ContextMenuBackend.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/SrcPlatform/Win32/ImageList.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(${TargetName} PRIVATE OIV_TEST_PLATFORM_LINUX)
    target_sources(${TargetName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/SrcPlatform/Linux/ContextMenuBackend.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/SrcPlatform/Linux/ImageList.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/SrcPlatform/Linux/LinuxKeyBindings.cpp
    )
endif()
target_include_directories(${TargetName} PRIVATE ${ExternalFolder}/json/include)
target_include_directories(${TargetName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer)
target_include_directories(${TargetName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/Source)
target_compile_definitions(${TargetName} PRIVATE OIV_TEST_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\")
target_link_libraries(${TargetName} PRIVATE Catch2::Catch2WithMain oivshared oivappcore ImageCodec tiff LWSLib)

if(OIV_BUILD_RENDERER_GL)
    target_compile_definitions(${TargetName} PRIVATE OIV_TEST_GL_RENDERER)
    target_link_libraries(${TargetName} PRIVATE OIVGLRenderer)
endif()

if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_link_options(${TargetName} PRIVATE "/SUBSYSTEM:CONSOLE")
    target_link_options(${TargetName} PRIVATE "/ENTRY:wmainCRTStartup")
elseif(MINGW)
    set_target_properties(${TargetName} PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(${TargetName} PRIVATE "-mconsole" "-municode")
endif()
