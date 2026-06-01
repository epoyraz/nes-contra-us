cmake_minimum_required(VERSION 3.21)

project(contra_port LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(CONTRA_PORT_BUILD_SDL "Build the SDL2 host executable" ON)
option(CONTRA_PORT_FETCH_SDL "Fetch SDL2 from the official repository when it is not installed" ON)
option(CONTRA_PORT_BUILD_TESTS "Build native core regression tests" ON)
option(CONTRA_PORT_ENABLE_COVERAGE "Build GCC/Clang coverage instrumentation for native targets" OFF)
option(CONTRA_PORT_ENABLE_MESEN_TRACE_TESTS "Run Mesen original-ROM checkpoint trace export tests when Mesen and contra.nes are available" ON)

include(CTest)

add_subdirectory(port)
