cmake_minimum_required(VERSION 3.21)

project(contra_port LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(CONTRA_PORT_BUILD_SDL "Build the SDL2 host executable" ON)
option(CONTRA_PORT_FETCH_SDL "Fetch SDL2 from the official repository when it is not installed" ON)

add_subdirectory(port)
