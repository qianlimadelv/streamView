add_executable(streamview
    src/main.cpp
)

target_link_libraries(streamview
    PRIVATE
        StreamView::Bitstream
        StreamView::Export
)

target_compile_features(streamview PRIVATE cxx_std_20)
