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

target_compile_features(streamview PRIVATE cxx_std_20)
target_compile_definitions(streamview PRIVATE STREAMVIEW_VERSION="${PROJECT_VERSION}")
