#OIVD3DRenderer
cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE sourceFiles
    "./Source/*.h"
    "./Source/*.cpp"
)

set(TargetName OIVD3D11Renderer)
add_library (${TargetName} ${sourceFiles} )
foreach(library dxgi d3dcompiler d3d11 dxguid)
    find_library(OIV_${library}_LIBRARY NAMES ${library} HINTS ENV LIB ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
    if(NOT OIV_${library}_LIBRARY)
        message(FATAL_ERROR "D3D11 requires the target ${library} library. Install the Windows SDK/toolchain or set OIV_BUILD_RENDERER_D3D11=OFF.")
    endif()
    target_link_libraries(${TargetName} PRIVATE "${OIV_${library}_LIBRARY}")
endforeach()
target_link_libraries(${TargetName} PRIVATE LLUtils::LLUtils)
