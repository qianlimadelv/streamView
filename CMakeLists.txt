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
        # Optional: libde265 for HEVC block-level overlays (Phase 9b). A stale
        # pkg-config cache can report the library while its headers have gone
        # away (for example after a temporary spike directory is removed), so
        # require the public header before enabling the code path.
        pkg_check_modules(LIBDE265 QUIET IMPORTED_TARGET libde265)
        if(LIBDE265_FOUND)
            find_path(LIBDE265_INCLUDE_DIR
                NAMES libde265/de265.h
                HINTS ${LIBDE265_INCLUDE_DIRS}
            )
            if(LIBDE265_INCLUDE_DIR AND EXISTS "${LIBDE265_INCLUDE_DIR}/libde265/de265.h")
                set_property(TARGET PkgConfig::LIBDE265 APPEND PROPERTY
                    INTERFACE_INCLUDE_DIRECTORIES "${LIBDE265_INCLUDE_DIR}")
                message(STATUS "libde265 found; HEVC block-level overlays enabled")
            else()
                set(LIBDE265_FOUND FALSE)
                message(STATUS "libde265 headers not found; HEVC block-level overlays disabled")
            endif()
        else()
            message(STATUS "libde265 not found; HEVC block-level overlays disabled")
        endif()
    endif()

    # Fallback for platforms without pkg-config (e.g. Windows/MSVC): locate FFmpeg
    # from a supplied FFMPEG_ROOT and build an equivalent PkgConfig::FFMPEG target.
    if(NOT FFMPEG_FOUND AND DEFINED FFMPEG_ROOT)
        find_path(FF_INC libavformat/avformat.h HINTS "${FFMPEG_ROOT}/include")
        find_library(FF_AVFORMAT NAMES avformat HINTS "${FFMPEG_ROOT}/lib")
        find_library(FF_AVCODEC NAMES avcodec HINTS "${FFMPEG_ROOT}/lib")
        find_library(FF_AVUTIL NAMES avutil HINTS "${FFMPEG_ROOT}/lib")
        if(FF_INC AND FF_AVFORMAT AND FF_AVCODEC AND FF_AVUTIL)
            add_library(PkgConfig::FFMPEG INTERFACE IMPORTED)
            set_target_properties(PkgConfig::FFMPEG PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${FF_INC}"
                INTERFACE_LINK_LIBRARIES "${FF_AVFORMAT};${FF_AVCODEC};${FF_AVUTIL}")
            set(FFMPEG_FOUND TRUE)
            message(STATUS "FFmpeg found via FFMPEG_ROOT=${FFMPEG_ROOT}")
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
