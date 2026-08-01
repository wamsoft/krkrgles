cmake_minimum_required(VERSION 3.16)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_compile_options("$<$<AND:$<C_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:C>>:/utf-8>")
    add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/utf-8>")
    add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/Zc:__cplusplus>")
    project(dummy)
endif()

set(TPSTUB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../tp_stub CACHE PATH "Path to tp_stub directory")
include(${TPSTUB_DIR}/krkrz.cmake)

# 同一ビルドツリー内の他モジュール (motiondriver / live2d 等) が既に同構成の
# glad (egl=1.5 + gles2=3.1) を定義していれば共有する
if(NOT TARGET glad)
add_subdirectory(glad)
endif()

krkrz_plugin(krkrgles
    NCBIND
    SOURCES
        src/GLES.cpp
        src/GLClip.h
        src/GLClip.cpp
        src/GLEffect.h
        src/GLEffect.cpp
        src/GLFrameBufferObject.h
        src/GLFrameBufferObject.cpp
        src/GLShaderUtil.h
        src/GLShaderUtil.cpp
        src/GLTexture.h
        src/GLTexture.cpp
        src/OpenGLError.cpp
    LIBRARIES
        glad        
)