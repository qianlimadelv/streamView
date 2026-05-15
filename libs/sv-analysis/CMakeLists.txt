add_library(sv-analysis
    src/gop_builder.cpp
    src/h264_annex_b_analyzer.cpp
    src/h265_annex_b_analyzer.cpp
)

add_library(StreamView::Analysis ALIAS sv-analysis)

target_include_directories(sv-analysis
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sv-analysis
    PUBLIC
        StreamView::Bitstream
)

target_compile_features(sv-analysis PUBLIC cxx_std_20)
