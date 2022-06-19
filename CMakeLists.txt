# Copyright 2021 Max Waine

# This code is licenced under the ISC Licene, which can be found at https://www.isc.org/licenses/

cmake_minimum_required(VERSION 3.11)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "cardboard")

project(cardboard LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(ExternalLibraries)

add_executable(cardboard
    source/draw32.cpp
    source/error.cpp
    source/error.h
    source/light.h
    source/main.cpp
    source/mapdata.cpp
    source/mapdata.h
    source/matrix.h
    source/pixelmath.h
    source/rect.cpp
    source/rect.h
    source/render.cpp
    source/render.h
    source/vblit.cpp
    source/vectors.h
    source/video.cpp
    source/video.h
    source/visplane.cpp
    source/visplane.h
)

target_link_libraries(cardboard PRIVATE
    ${SDL2_LIBRARY}
)

target_include_directories(cardboard SYSTEM PRIVATE
    ${SDL2_INCLUDE_DIR}
)

set_target_properties(cardboard PROPERTIES
    CXX_STANDARD 11
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)