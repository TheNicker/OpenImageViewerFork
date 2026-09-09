#OIV library
cmake_minimum_required(VERSION 3.10)



#todo fix it.
set(ExternalFolder ../External)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(_default_d3d11 ON)
    set(_default_gl OFF)
    set(_default_vk OFF)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_default_d3d11 OFF)
    set(_default_gl ON)
    set(_default_vk ON)
else()
    set(_default_d3d11 OFF)
    set(_default_gl OFF)
    set(_default_vk OFF)
endif()

option(OIV_BUILD_RENDERER_D3D11 "Build D3D11 renderer" ${_default_d3d11})
option(OIV_BUILD_RENDERER_GL "Build OpenGL renderer" ${_default_gl})
option(OIV_BUILD_RENDERER_VK "Build Vulkan renderer" ${_default_vk})


include_directories(./OIV/Include)
if(OIV_BUILD_RENDERER_D3D11)
    include_directories(Renderers/OIVD3D11Renderer/Include)
endif()
if(OIV_BUILD_RENDERER_GL)
    include_directories(Renderers/OIVGLRenderer)
endif()
if(OIV_BUILD_RENDERER_VK)
    include_directories(Renderers/OIVVKRenderer)
endif()
include_directories(${ExternalFolder}/ExoticNumbers/include)
include_directories(${ExternalFolder}/ImageCodec/ImageCodec/Include)

add_subdirectory(OIV)
add_subdirectory(Renderers)
