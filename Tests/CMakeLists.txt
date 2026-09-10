
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/Source/CommandLine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/Source/ImageList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/Source/MouseGestureController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/Source/MouseMultiClickHandler.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_include_directories(${TargetName} PRIVATE ${ExternalFolder}/LInput/Include)
    target_sources(${TargetName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/SrcPlatform/Win32/ContextMenuBackend.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(${TargetName} PRIVATE OIV_TEST_PLATFORM_LINUX)
    if(NOT TARGET PkgConfig::GTK3)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
    endif()
    pkg_check_modules(WaylandClient REQUIRED IMPORTED_TARGET wayland-client)
    target_link_libraries(${TargetName} PRIVATE PkgConfig::GTK3 PkgConfig::WaylandClient)
    target_sources(${TargetName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/SrcPlatform/Linux/PlatformFileDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/SrcPlatform/Linux/ContextMenuBackend.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/SrcPlatform/Linux/LinuxKeyBindings.cpp
    )
endif()
target_include_directories(${TargetName} PRIVATE ${ExternalFolder}/json/include)
target_include_directories(${TargetName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer)
target_include_directories(${TargetName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Clients/OIViewer/Source)
target_include_directories(${TargetName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../OIVLib/OIV/Source)
target_compile_definitions(${TargetName} PRIVATE OIV_TEST_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\")
target_link_libraries(${TargetName} PRIVATE CLI11::CLI11 Catch2::Catch2WithMain oivshared oivappcore ImageCodec tiff LWSLib)

if(OIV_BUILD_RENDERER_GL)
    target_compile_definitions(${TargetName} PRIVATE OIV_TEST_GL_RENDERER)
    target_link_libraries(${TargetName} PRIVATE OIVGLRenderer)
endif()

if(OIV_BUILD_RENDERER_VK)
    find_package(Vulkan REQUIRED)
    target_compile_definitions(${TargetName} PRIVATE OIV_TEST_VK_RENDERER)
    target_link_libraries(${TargetName} PRIVATE OIVVKRenderer)

    # Keep Vulkan failure doubles isolated from the normal renderer tests and the real Vulkan loader.
    add_executable(tests_vk_failures
        Vulkan/FailureTests.cpp
        ../OIVLib/Renderers/OIVVKRenderer/Source/VKContext.cpp
        ../OIVLib/Renderers/OIVVKRenderer/Source/VKPipeline.cpp
        ../OIVLib/Renderers/OIVVKRenderer/Source/VKRenderer.cpp
        ../OIVLib/Renderers/OIVVKRenderer/Source/VKTexture.cpp
    )
    add_dependencies(tests_vk_failures VKShaders)
    target_compile_definitions(tests_vk_failures PRIVATE $<TARGET_PROPERTY:OIVVKRenderer,COMPILE_DEFINITIONS>)
    if(WIN32)
        target_link_libraries(tests_vk_failures PRIVATE user32)
    endif()
    target_include_directories(tests_vk_failures PRIVATE
        ../OIVLib/Renderers/OIVVKRenderer/Source
        ../OIVLib/OIV/Include
    )
    target_link_libraries(tests_vk_failures PRIVATE Catch2::Catch2WithMain ImageCodec ImageUtil LLUtils::LLUtils Vulkan::Headers)
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_link_options(tests_vk_failures PRIVATE "/SUBSYSTEM:CONSOLE" "/ENTRY:wmainCRTStartup")
    elseif(MINGW)
        target_link_options(tests_vk_failures PRIVATE "-mconsole" "-municode")
    endif()
endif()

if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_link_options(${TargetName} PRIVATE "/SUBSYSTEM:CONSOLE")
    target_link_options(${TargetName} PRIVATE "/ENTRY:wmainCRTStartup")
elseif(MINGW)
    set_target_properties(${TargetName} PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(${TargetName} PRIVATE "-mconsole" "-municode")
endif()
