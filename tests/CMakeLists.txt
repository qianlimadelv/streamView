add_executable(sv-bitstream-tests
    bitstream_tests.cpp
)

target_link_libraries(sv-bitstream-tests
    PRIVATE
        StreamView::Bitstream
)

target_compile_features(sv-bitstream-tests PRIVATE cxx_std_20)

add_test(NAME sv-bitstream-tests COMMAND sv-bitstream-tests)

add_executable(sv-analysis-tests
    analysis_tests.cpp
)

target_link_libraries(sv-analysis-tests
    PRIVATE
        StreamView::Analysis
)

target_compile_features(sv-analysis-tests PRIVATE cxx_std_20)

add_test(NAME sv-analysis-tests COMMAND sv-analysis-tests)
