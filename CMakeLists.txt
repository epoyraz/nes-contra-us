cmake_minimum_required(VERSION 3.21)

project(contra_port LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(CONTRA_PORT_BUILD_SDL "Build the SDL2 host executable" ON)
option(CONTRA_PORT_FETCH_SDL "Fetch SDL2 from the official repository when it is not installed" ON)
option(CONTRA_PORT_BUILD_TESTS "Build native core regression tests" ON)
option(CONTRA_PORT_ENABLE_COVERAGE "Build GCC/Clang coverage instrumentation for native targets" OFF)
option(CONTRA_PORT_ENABLE_MESEN_TRACE_TESTS "Run Mesen original-ROM checkpoint trace export tests when Mesen and a ROM are available" ON)
option(CONTRA_PORT_REQUIRE_MESEN_TRACE_TESTS "Fail configuration when Mesen trace tests cannot run" OFF)
option(CONTRA_PORT_REQUIRE_VISUAL_PARITY_TEST "Fail configuration when the indoor visual parity test cannot run" OFF)
set(CONTRA_PORT_VISUAL_MESEN_RECORDING "" CACHE FILEPATH "Mesen input recording for the indoor visual parity test")
set(CONTRA_PORT_VISUAL_NATIVE_RECORDING "" CACHE FILEPATH "Native input recording for the indoor visual parity test (defaults to the Mesen recording)")
set(CONTRA_PORT_VISUAL_FRAME "9321" CACHE STRING "Indoor frame to compare visually")
set(CONTRA_PORT_VISUAL_MAX_MISMATCHES "1200" CACHE STRING "Maximum accepted palette-independent pixel differences")

if(CONTRA_PORT_REQUIRE_MESEN_TRACE_TESTS AND NOT CONTRA_PORT_ENABLE_MESEN_TRACE_TESTS)
    message(FATAL_ERROR "CONTRA_PORT_REQUIRE_MESEN_TRACE_TESTS requires CONTRA_PORT_ENABLE_MESEN_TRACE_TESTS=ON")
endif()
if(CONTRA_PORT_REQUIRE_VISUAL_PARITY_TEST AND NOT CONTRA_PORT_ENABLE_MESEN_TRACE_TESTS)
    message(FATAL_ERROR "CONTRA_PORT_REQUIRE_VISUAL_PARITY_TEST requires CONTRA_PORT_ENABLE_MESEN_TRACE_TESTS=ON")
endif()
if((CONTRA_PORT_REQUIRE_MESEN_TRACE_TESTS OR CONTRA_PORT_REQUIRE_VISUAL_PARITY_TEST)
   AND NOT CONTRA_PORT_BUILD_TESTS)
    message(FATAL_ERROR "Required parity tests need CONTRA_PORT_BUILD_TESTS=ON")
endif()

include(CTest)

add_subdirectory(port)
