add_executable(streamview
    src/main.cpp
)

target_link_libraries(streamview
    PRIVATE
        StreamView::Export
)

if(TARGET StreamView::Demux)
    target_link_libraries(streamview
        PRIVATE
            StreamView::Demux
    )
    target_compile_definitions(streamview PRIVATE STREAMVIEW_HAS_FFMPEG_DEMUX=1)
endif()

if(TARGET StreamView::Decode)
    target_link_libraries(streamview
        PRIVATE
            StreamView::Decode
    )
    target_compile_definitions(streamview PRIVATE STREAMVIEW_HAS_FFMPEG_DECODE=1)
endif()

target_compile_features(streamview PRIVATE cxx_std_20)
target_compile_definitions(streamview PRIVATE STREAMVIEW_VERSION="${PROJECT_VERSION}")

# Windows: embed a UTF-8 manifest so argv, the CRT (file I/O) and FFmpeg all use
# UTF-8 — required to open non-ASCII (e.g. Chinese) file paths.
if(WIN32 AND MSVC)
    target_sources(streamview PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/app.manifest)
    target_compile_options(streamview PRIVATE /utf-8)
endif()

install(TARGETS streamview
    RUNTIME DESTINATION bin
)
