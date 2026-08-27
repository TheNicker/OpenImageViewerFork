if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "OIVGLRenderer currently supports Windows and Linux.")
endif()

include(FetchContent)

FetchContent_Declare(
    oiv_glew
    URL https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.zip
    URL_HASH SHA256=a9046a913774395a095edcc0b0ac2d81c3aacca61787b39839b941e9be14e0d4
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR src
)
FetchContent_MakeAvailable(oiv_glew)

if(WIN32)
    find_package(OpenGL REQUIRED)
    set(oivOpenGLLibraries OpenGL::GL)
    set(glContextSource GLContext.cpp)
else()
    get_target_property(oivLwsCompileDefinitions LWSLib INTERFACE_COMPILE_DEFINITIONS)
    if("LWS_PLATFORM_WAYLAND" IN_LIST oivLwsCompileDefinitions)
        find_package(OpenGL REQUIRED COMPONENTS OpenGL EGL)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(WaylandEGL REQUIRED IMPORTED_TARGET wayland-egl)
        set(oivOpenGLLibraries OpenGL::OpenGL OpenGL::EGL PkgConfig::WaylandEGL)
        set(glContextSource GLContextWayland.cpp)
        set(oivGLWayland ON)
    elseif("LWS_PLATFORM_X11" IN_LIST oivLwsCompileDefinitions)
        find_package(OpenGL REQUIRED COMPONENTS OpenGL GLX)
        find_package(X11 REQUIRED)
        set(oivOpenGLLibraries OpenGL::OpenGL OpenGL::GLX X11::X11 ${CMAKE_DL_LIBS})
        set(glContextSource GLContextX11.cpp)
    else()
        message(FATAL_ERROR "OIVGLRenderer requires an LWS X11 or Wayland backend on Linux.")
    endif()
endif()

add_library(oiv_glew STATIC
    ${oiv_glew_SOURCE_DIR}/include/GL/glew.h
    ${oiv_glew_SOURCE_DIR}/include/GL/glxew.h
    ${oiv_glew_SOURCE_DIR}/include/GL/wglew.h
    ${oiv_glew_SOURCE_DIR}/src/glew.c
)
target_compile_definitions(oiv_glew
    PUBLIC
        GLEW_STATIC
        GLEW_NO_GLU
)
target_include_directories(oiv_glew SYSTEM PUBLIC ${oiv_glew_SOURCE_DIR}/include)
target_link_libraries(oiv_glew PUBLIC ${oivOpenGLLibraries})
if(oivGLWayland)
    target_compile_definitions(oiv_glew PRIVATE GLEW_EGL)
endif()

add_library(OIVGLRenderer STATIC
    ${glContextSource}
    GLContext.h
    GLRenderer/GlError.h
    GLRenderer/GLGpuProgram.h
    GLRenderer/GLTexture.h
    GLRenderer/Quad.h
    OIVGLRenderer.cpp
    OIVGLRenderer.h
    OIVGLRendererFactory.cpp
    OIVGLRendererFactory.h
)
target_include_directories(OIVGLRenderer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../OIV/Include
)
target_link_libraries(OIVGLRenderer
    PUBLIC
        ImageCodec
        LLUtils::LLUtils
        oiv_glew
    PRIVATE
        ImageUtil
        ${oivOpenGLLibraries}
)
if(oivGLWayland)
    target_compile_definitions(OIVGLRenderer PRIVATE OIV_GL_WAYLAND)
endif()
