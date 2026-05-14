add_library(sv-demux
    src/ffmpeg_h264_demuxer.cpp
)

add_library(StreamView::Demux ALIAS sv-demux)

target_include_directories(sv-demux
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sv-demux
    PUBLIC
        StreamView::Core
    PRIVATE
        PkgConfig::FFMPEG
)

target_compile_features(sv-demux PUBLIC cxx_std_20)
