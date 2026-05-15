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

add_test(
    NAME streamview-cli-golden-h264-minimal
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/h264_minimal_sps_pps_idr.json
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_minimal_sps_pps_idr.actual.json
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_golden_test.cmake
)

set_tests_properties(streamview-cli-golden-h264-minimal
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-golden-h265-minimal
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h265/minimal_vps_sps_pps_idr.h265
        -DSTREAMVIEW_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/h265_minimal_vps_sps_pps_idr.json
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h265_minimal_vps_sps_pps_idr.actual.json
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_golden_test.cmake
)

set_tests_properties(streamview-cli-golden-h265-minimal
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
