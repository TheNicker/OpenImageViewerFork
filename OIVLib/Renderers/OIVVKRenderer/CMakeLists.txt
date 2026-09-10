cmake_minimum_required(VERSION 3.10)

# Headers are target-independent; the shader compiler must run on the build host.
find_path(OIV_VULKAN_INCLUDE_DIR vulkan/vulkan.h HINTS "$ENV{VULKAN_SDK}" PATH_SUFFIXES Include include)
find_program(OIV_GLSLC glslc HINTS "$ENV{VULKAN_SDK}" PATH_SUFFIXES Bin bin NO_CMAKE_FIND_ROOT_PATH)
if(NOT OIV_VULKAN_INCLUDE_DIR OR NOT EXISTS "${OIV_VULKAN_INCLUDE_DIR}/vulkan/vulkan.h" OR NOT OIV_GLSLC)
    message(FATAL_ERROR "Vulkan requires SDK headers and a host glslc. Set VULKAN_SDK/OIV_VULKAN_INCLUDE_DIR/OIV_GLSLC or disable OIV_BUILD_RENDERER_VK.")
endif()
execute_process(COMMAND "${OIV_GLSLC}" --version RESULT_VARIABLE glslcResult OUTPUT_QUIET ERROR_QUIET TIMEOUT 10)
if(NOT glslcResult EQUAL 0)
    message(FATAL_ERROR "OIV_GLSLC must run on the build host. Choose a host glslc or set OIV_BUILD_RENDERER_VK=OFF.")
endif()
if(NOT WIN32)
    find_package(Vulkan QUIET GLOBAL)
    if(NOT Vulkan_FOUND)
        message(FATAL_ERROR "Vulkan target libraries are missing. Install the Vulkan development package or set OIV_BUILD_RENDERER_VK=OFF.")
    endif()
endif()
if(NOT TARGET Vulkan::Headers)
    add_library(Vulkan::Headers INTERFACE IMPORTED GLOBAL)
    set_target_properties(Vulkan::Headers PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OIV_VULKAN_INCLUDE_DIR}")
endif()

file(GLOB_RECURSE sourceFiles CONFIGURE_DEPENDS
    "./Source/*.h"
    "./Source/*.cpp"
)
set(OIV_PROGRAM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../OIV/Resources/Programs)
file(GLOB shaderDependencies CONFIGURE_DEPENDS "${OIV_PROGRAM_DIR}/*.shader")

set(TargetName OIVVKRenderer)
list(FILTER sourceFiles EXCLUDE REGEX "VKRuntimeWindows\\.cpp$")
add_library(${TargetName} STATIC ${sourceFiles} ${shaderDependencies})
if(WIN32)
    target_sources(${TargetName} PRIVATE Source/VKRuntimeWindows.cpp)
endif()

set(SHADER_CACHE_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ShaderCache)
file(MAKE_DIRECTORY ${SHADER_CACHE_DIR})

set(shaderSpecs
    "QuadVP.shader|vert|QuadVP.vert.spv"
    "QuadFP.shader|frag|QuadFP.frag.spv"
    "QuadSelectionFP.shader|frag|QuadSelectionFP.frag.spv"
    "QuadSimpleFP.shader|frag|QuadSimpleFP.frag.spv"
)

foreach(shaderSpec ${shaderSpecs})
    string(REPLACE "|" ";" shaderSpecParts ${shaderSpec})
    list(GET shaderSpecParts 0 shaderName)
    list(GET shaderSpecParts 1 shaderStage)
    list(GET shaderSpecParts 2 spvName)

    set(shaderFile ${OIV_PROGRAM_DIR}/${shaderName})
    set(spvFile ${SHADER_CACHE_DIR}/${spvName})
    add_custom_command(
        OUTPUT ${spvFile}
        COMMAND "${OIV_GLSLC}"
            -x glsl
            -std=450core
            -DVK=1
            -fshader-stage=${shaderStage}
            -I ${OIV_PROGRAM_DIR}
            ${shaderFile}
            -o ${spvFile}
        DEPENDS ${shaderDependencies}
        COMMENT "Compiling ${shaderName} to SPIR-V"
        VERBATIM
    )
    list(APPEND spvFiles ${spvFile})
endforeach()

add_custom_target(VKShaders ALL DEPENDS ${spvFiles})
add_dependencies(${TargetName} VKShaders)

target_include_directories(${TargetName}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../OIV/Include
)

target_link_libraries(${TargetName}
    PUBLIC
        ImageCodec
        LLUtils::LLUtils
    PRIVATE
        Vulkan::Headers
        ImageUtil
)

if(WIN32)
    target_link_libraries(${TargetName} PRIVATE user32)
    target_compile_definitions(${TargetName} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(${TargetName} PRIVATE Vulkan::Vulkan)
    get_target_property(oivLwsCompileDefinitions LWSLib INTERFACE_COMPILE_DEFINITIONS)
    if("LWS_PLATFORM_WAYLAND" IN_LIST oivLwsCompileDefinitions)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(Wayland REQUIRED IMPORTED_TARGET wayland-client)
        target_link_libraries(${TargetName} PRIVATE PkgConfig::Wayland)
        target_compile_definitions(${TargetName} PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
    elseif("LWS_PLATFORM_X11" IN_LIST oivLwsCompileDefinitions)
        find_package(X11 REQUIRED)
        target_link_libraries(${TargetName} PRIVATE X11::X11)
        target_compile_definitions(${TargetName} PRIVATE VK_USE_PLATFORM_XLIB_KHR)
    else()
        message(FATAL_ERROR "OIVVKRenderer requires an LWS X11 or Wayland backend on Linux.")
    endif()
endif()
