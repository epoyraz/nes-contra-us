function(contra_apply_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

function(contra_apply_coverage target_name)
    if(CONTRA_PORT_ENABLE_COVERAGE)
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target_name} PRIVATE -O0 -g --coverage)
            target_link_options(${target_name} PRIVATE --coverage)
        else()
            message(WARNING "CONTRA_PORT_ENABLE_COVERAGE is only wired for GCC/Clang.")
        endif()
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
contra_apply_coverage(contra_core)

add_executable(contra_port_headless
    tools/headless_main.c
)

target_link_libraries(contra_port_headless PRIVATE contra_core)

contra_apply_warnings(contra_port_headless)
contra_apply_coverage(contra_port_headless)

if(CONTRA_PORT_BUILD_TESTS)
    add_executable(contra_core_tests
        tests/core_behavior_tests.c
    )

    target_link_libraries(contra_core_tests PRIVATE contra_core)

    contra_apply_warnings(contra_core_tests)
    contra_apply_coverage(contra_core_tests)

    add_test(NAME contra_core_tests COMMAND contra_core_tests)

    add_executable(contra_checkpoint_trace_tests
        tests/checkpoint_trace_tests.c
    )

    target_link_libraries(contra_checkpoint_trace_tests PRIVATE contra_core)

    contra_apply_warnings(contra_checkpoint_trace_tests)
    contra_apply_coverage(contra_checkpoint_trace_tests)

    add_test(NAME contra_checkpoint_trace_tests COMMAND contra_checkpoint_trace_tests)

    add_executable(contra_coverage_manifest_tests
        tests/coverage_manifest_tests.c
    )

    contra_apply_warnings(contra_coverage_manifest_tests)
    contra_apply_coverage(contra_coverage_manifest_tests)

    add_test(NAME contra_coverage_manifest_tests COMMAND contra_coverage_manifest_tests)
    set_tests_properties(contra_coverage_manifest_tests PROPERTIES
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()

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
        contra_apply_coverage(contra_port_sdl)
    else()
        message(WARNING "SDL2 was not found. Only contra_port_headless will be built.")
    endif()
endif()
