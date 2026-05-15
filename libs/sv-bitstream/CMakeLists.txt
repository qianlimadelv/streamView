add_library(sv-bitstream
    src/annex_b.cpp
    src/bit_reader.cpp
    src/h264_nal.cpp
    src/h264_pps.cpp
    src/h264_slice.cpp
    src/h264_sps.cpp
    src/h265_nal.cpp
    src/h265_pps.cpp
    src/h265_slice.cpp
    src/h265_sps.cpp
    src/h265_vps.cpp
    src/rbsp.cpp
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
