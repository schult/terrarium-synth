cmake_minimum_required(VERSION 3.20)
project(TerrariumSynth VERSION 1.0.0)

set(FIRMWARE_NAME TerrariumSynth)
set(FIRMWARE_SOURCES
    main.cpp
    syscalls.c
    util/Blink.h
    util/EffectState.h
    util/Led.h
    util/Led.cpp
    util/Terrarium.h
    util/Terrarium.cpp
    util/TriangleSynth.h
)
set(LIBDAISY_DIR ${CMAKE_SOURCE_DIR}/lib/libDaisy)
include(${LIBDAISY_DIR}/cmake/default_build.cmake)

option(Q_BUILD_EXAMPLES "build Q library examples" OFF)
option(Q_BUILD_TEST "build Q library tests" OFF)
option(Q_BUILD_IO "build Q IO library" OFF)
add_subdirectory(lib/q)
target_link_libraries(${FIRMWARE_NAME} PUBLIC libq)

target_include_directories(${FIRMWARE_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

set_target_properties(${FIRMWARE_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_options(${FIRMWARE_NAME} PRIVATE
    -flto=auto
)
