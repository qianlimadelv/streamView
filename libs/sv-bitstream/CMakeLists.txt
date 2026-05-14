add_library(sv-bitstream
    src/annex_b.cpp
    src/h264_nal.cpp
)

add_library(StreamView::Bitstream ALIAS sv-bitstream)

target_include_directories(sv-bitstream
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sv-bitstream
    PUBLIC
        StreamView::Core
)

target_compile_features(sv-bitstream PUBLIC cxx_std_20)
