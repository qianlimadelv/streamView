add_library(sv-decode
    src/frame_decoder.cpp
    src/block_overlay.cpp
)

add_library(StreamView::Decode ALIAS sv-decode)

target_include_directories(sv-decode
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sv-decode
    PUBLIC
        StreamView::Core
    PRIVATE
        PkgConfig::FFMPEG
)

# Optional: libde265 enables HEVC block-level overlays (QP/partition/intra/MV).
if(TARGET PkgConfig::LIBDE265)
    target_link_libraries(sv-decode PRIVATE PkgConfig::LIBDE265)
    target_compile_definitions(sv-decode PRIVATE STREAMVIEW_HAS_LIBDE265=1)
endif()

target_compile_features(sv-decode PUBLIC cxx_std_20)
