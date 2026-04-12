function(contra_apply_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

add_library(contra_core STATIC
    contra_core/src/core.c
)

target_include_directories(contra_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/contra_core/include
)

contra_apply_warnings(contra_core)

add_executable(contra_port_headless
    tools/headless_main.c
)

target_link_libraries(contra_port_headless PRIVATE contra_core)

contra_apply_warnings(contra_port_headless)

if(CONTRA_PORT_BUILD_SDL)
    find_package(SDL2 CONFIG QUIET)

    if(NOT SDL2_FOUND AND CONTRA_PORT_FETCH_SDL)
        include(FetchContent)

        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
        set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)

        FetchContent_Declare(
            SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-2.32.10
            GIT_SHALLOW TRUE
        )

        FetchContent_MakeAvailable(SDL2)
    endif()

    set(CONTRA_SDL_TARGET "")

    if(TARGET SDL2::SDL2)
        set(CONTRA_SDL_TARGET SDL2::SDL2)
    elseif(TARGET SDL2::SDL2-static)
        set(CONTRA_SDL_TARGET SDL2::SDL2-static)
    elseif(TARGET SDL2-static)
        set(CONTRA_SDL_TARGET SDL2-static)
    endif()

    if(CONTRA_SDL_TARGET)
        add_executable(contra_port_sdl
            platform_sdl/main.c
        )

        target_link_libraries(contra_port_sdl
            PRIVATE
                contra_core
                ${CONTRA_SDL_TARGET}
        )

        contra_apply_warnings(contra_port_sdl)
    else()
        message(WARNING "SDL2 was not found. Only contra_port_headless will be built.")
    endif()
endif()
