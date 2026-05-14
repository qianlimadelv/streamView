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
    endif()

    if(FFMPEG_FOUND)
        add_subdirectory(libs/sv-demux)
        set(STREAMVIEW_HAS_FFMPEG_DEMUX ON)
    else()
        message(STATUS "FFmpeg development libraries not found; sv-demux is disabled")
    endif()
endif()
add_subdirectory(libs/sv-export)
add_subdirectory(apps/streamview-cli)

if(STREAMVIEW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
