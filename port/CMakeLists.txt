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

add_executable(contra_level1_frame_trace
    tools/level1_frame_trace.c
)

target_link_libraries(contra_level1_frame_trace PRIVATE contra_core)

contra_apply_warnings(contra_level1_frame_trace)
contra_apply_coverage(contra_level1_frame_trace)

add_executable(contra_play_replay_trace
    tools/play_replay_trace.c
)

target_link_libraries(contra_play_replay_trace PRIVATE contra_core)

contra_apply_warnings(contra_play_replay_trace)
contra_apply_coverage(contra_play_replay_trace)

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
    add_test(
        NAME contra_checkpoint_trace_export
        COMMAND contra_checkpoint_trace_tests --emit-jsonl "${CMAKE_CURRENT_BINARY_DIR}/contra_checkpoint_trace.jsonl"
    )

    add_executable(contra_checkpoint_trace_export_tests
        tests/checkpoint_trace_export_tests.c
    )

    contra_apply_warnings(contra_checkpoint_trace_export_tests)
    contra_apply_coverage(contra_checkpoint_trace_export_tests)

    add_test(
        NAME contra_checkpoint_trace_export_tests
        COMMAND contra_checkpoint_trace_export_tests "${CMAKE_CURRENT_BINARY_DIR}/contra_checkpoint_trace.jsonl"
    )
    set_tests_properties(contra_checkpoint_trace_export_tests PROPERTIES
        DEPENDS contra_checkpoint_trace_export
    )

    add_executable(contra_checkpoint_trace_compare
        tests/checkpoint_trace_compare.c
    )

    contra_apply_warnings(contra_checkpoint_trace_compare)
    contra_apply_coverage(contra_checkpoint_trace_compare)

    add_test(
        NAME contra_checkpoint_trace_compare_self
        COMMAND contra_checkpoint_trace_compare
            "${CMAKE_CURRENT_BINARY_DIR}/contra_checkpoint_trace.jsonl"
            "${CMAKE_CURRENT_BINARY_DIR}/contra_checkpoint_trace.jsonl"
    )
    set_tests_properties(contra_checkpoint_trace_compare_self PROPERTIES
        DEPENDS contra_checkpoint_trace_export
    )

    add_executable(contra_coverage_manifest_tests
        tests/coverage_manifest_tests.c
    )

    contra_apply_warnings(contra_coverage_manifest_tests)
    contra_apply_coverage(contra_coverage_manifest_tests)

    add_test(NAME contra_coverage_manifest_tests COMMAND contra_coverage_manifest_tests)
    set_tests_properties(contra_coverage_manifest_tests PROPERTIES
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )

    if(CONTRA_PORT_ENABLE_MESEN_TRACE_TESTS)
        find_program(CONTRA_MESEN_EXECUTABLE NAMES Mesen.exe Mesen mesen)
        set(CONTRA_MESEN_TRACE_JSONL "${CMAKE_CURRENT_BINARY_DIR}/mesen_checkpoint_trace.jsonl")

        if(CONTRA_MESEN_EXECUTABLE AND EXISTS "${CMAKE_SOURCE_DIR}/contra.nes")
            add_executable(contra_mesen_trace_export_tests
                tests/mesen_trace_export_tests.c
            )

            contra_apply_warnings(contra_mesen_trace_export_tests)
            contra_apply_coverage(contra_mesen_trace_export_tests)

            add_executable(contra_checkpoint_trace_compare_attract_semantics
                tests/checkpoint_trace_compare_attract_semantics.c
            )

            contra_apply_warnings(contra_checkpoint_trace_compare_attract_semantics)
            contra_apply_coverage(contra_checkpoint_trace_compare_attract_semantics)

            add_executable(contra_checkpoint_trace_compare_level2_room_chain_semantics
                tests/checkpoint_trace_compare_level2_room_chain_semantics.c
            )

            contra_apply_warnings(contra_checkpoint_trace_compare_level2_room_chain_semantics)
            contra_apply_coverage(contra_checkpoint_trace_compare_level2_room_chain_semantics)

            add_test(
                NAME contra_mesen_checkpoint_trace_export
                COMMAND
                    "${CMAKE_COMMAND}" -E env
                    "CONTRA_MESEN_TRACE_JSONL=${CONTRA_MESEN_TRACE_JSONL}"
                    "${CONTRA_MESEN_EXECUTABLE}"
                    --testRunner
                    --enableStdout
                    --doNotSaveSettings
                    --timeout=30
                    --debug.scriptWindow.allowIoOsAccess=true
                    "${CMAKE_SOURCE_DIR}/tools/mesen_checkpoint_trace.lua"
                    "${CMAKE_SOURCE_DIR}/contra.nes"
            )

            add_test(
                NAME contra_mesen_trace_export_tests
                COMMAND contra_mesen_trace_export_tests "${CONTRA_MESEN_TRACE_JSONL}"
            )
            set_tests_properties(contra_mesen_trace_export_tests PROPERTIES
                DEPENDS contra_mesen_checkpoint_trace_export
            )

            add_test(
                NAME contra_checkpoint_trace_compare_attract_semantics
                COMMAND contra_checkpoint_trace_compare_attract_semantics
                    "${CONTRA_MESEN_TRACE_JSONL}"
                    "${CMAKE_CURRENT_BINARY_DIR}/contra_checkpoint_trace.jsonl"
            )
            set_tests_properties(contra_checkpoint_trace_compare_attract_semantics PROPERTIES
                DEPENDS "contra_mesen_checkpoint_trace_export;contra_checkpoint_trace_export"
            )

            add_test(
                NAME contra_checkpoint_trace_compare_level2_room_chain_semantics
                COMMAND contra_checkpoint_trace_compare_level2_room_chain_semantics
                    "${CONTRA_MESEN_TRACE_JSONL}"
                    "${CMAKE_CURRENT_BINARY_DIR}/contra_checkpoint_trace.jsonl"
            )
            set_tests_properties(contra_checkpoint_trace_compare_level2_room_chain_semantics PROPERTIES
                DEPENDS "contra_mesen_checkpoint_trace_export;contra_checkpoint_trace_export"
            )
        else()
            message(STATUS "Mesen original-ROM trace tests disabled: Mesen executable or contra.nes not found.")
        endif()
    endif()
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
