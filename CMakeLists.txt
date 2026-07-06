cmake_minimum_required(VERSION 3.24)

project(StreamView
    VERSION 0.1.0
    DESCRIPTION "Cross-platform compressed video bitstream analysis prototype"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(STREAMVIEW_BUILD_TESTS "Build StreamView tests" ON)
option(STREAMVIEW_ENABLE_FFMPEG "Enable FFmpeg-based demux/decode modules" ON)

add_subdirectory(libs/sv-core)
add_subdirectory(libs/sv-bitstream)
add_subdirectory(libs/sv-analysis)
if(STREAMVIEW_ENABLE_FFMPEG)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(FFMPEG QUIET IMPORTED_TARGET libavformat libavcodec libavutil)
        # Optional: libde265 for HEVC block-level overlays (Phase 9b).
        pkg_check_modules(LIBDE265 QUIET IMPORTED_TARGET libde265)
        if(LIBDE265_FOUND)
            message(STATUS "libde265 found; HEVC block-level overlays enabled")
        else()
            message(STATUS "libde265 not found; HEVC block-level overlays disabled")
        endif()
    endif()

    if(FFMPEG_FOUND)
        add_subdirectory(libs/sv-demux)
        add_subdirectory(libs/sv-decode)
        set(STREAMVIEW_HAS_FFMPEG_DEMUX ON)
    else()
        message(STATUS "FFmpeg development libraries not found; sv-demux and sv-decode are disabled")
    endif()
endif()
add_subdirectory(libs/sv-export)
add_subdirectory(apps/streamview-cli)

if(STREAMVIEW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(FILES
    README.md
    LICENSE
    DESTINATION .
)

install(FILES
    docs/BUILD.md
    docs/BUILD_CN.md
    docs/CLI.md
    docs/CLI_CN.md
    docs/CODEC-SCOPE.md
    docs/CODEC-SCOPE_CN.md
    docs/RELEASE.md
    docs/RELEASE_CN.md
    DESTINATION docs
)
