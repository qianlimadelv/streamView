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
    NAME streamview-cli-json-summary-h264-minimal
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/h264_minimal_summary.json
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_minimal_summary.actual.json
        -DSTREAMVIEW_JSON_MODE=summary
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_json_mode_golden_test.cmake
)

set_tests_properties(streamview-cli-json-summary-h264-minimal
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-limited-json-h264-minimal
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_minimal_limited.actual.json
        -DSTREAMVIEW_LIMIT_NALS=2
        -DSTREAMVIEW_EXPECTED_OMITTED=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_limited_json_test.cmake
)

set_tests_properties(streamview-cli-limited-json-h264-minimal
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-summary-h264-minimal
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/h264_minimal_summary.txt
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_minimal_summary.actual.txt
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_summary_golden_test.cmake
)

set_tests_properties(streamview-cli-summary-h264-minimal
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

add_test(
    NAME streamview-cli-json-summary-h265-minimal
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h265/minimal_vps_sps_pps_idr.h265
        -DSTREAMVIEW_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/h265_minimal_summary.json
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h265_minimal_summary.actual.json
        -DSTREAMVIEW_JSON_MODE=summary
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_json_mode_golden_test.cmake
)

set_tests_properties(streamview-cli-json-summary-h265-minimal
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-summary-h265-minimal
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h265/minimal_vps_sps_pps_idr.h265
        -DSTREAMVIEW_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/h265_minimal_summary.txt
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h265_minimal_summary.actual.txt
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_summary_golden_test.cmake
)

set_tests_properties(streamview-cli-summary-h265-minimal
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

set(STREAMVIEW_REAL_MEDIA_DIR "" CACHE PATH "Optional local directory that contains real H.264/H.265 samples for smoke tests")
if(NOT STREAMVIEW_REAL_MEDIA_DIR AND EXISTS "/home/zhangyp/code/media")
    set(STREAMVIEW_REAL_MEDIA_DIR "/home/zhangyp/code/media")
endif()

if(STREAMVIEW_REAL_MEDIA_DIR)
    set(STREAMVIEW_REAL_H264_SAMPLE "${STREAMVIEW_REAL_MEDIA_DIR}/video_264_hd.h264")
    if(EXISTS "${STREAMVIEW_REAL_H264_SAMPLE}")
        add_test(
            NAME streamview-cli-real-h264-smoke
            COMMAND
                ${CMAKE_COMMAND}
                -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
                -DSTREAMVIEW_SAMPLE=${STREAMVIEW_REAL_H264_SAMPLE}
                -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/real_h264_smoke.actual.json
                -DSTREAMVIEW_EXPECTED_CODEC=h264
                -DSTREAMVIEW_MIN_NAL_COUNT=100
                -DSTREAMVIEW_MIN_FRAME_COUNT=100
                -DSTREAMVIEW_REQUIRED_JSON_FIELD=active_sps
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_smoke_test.cmake
        )
    endif()

    set(STREAMVIEW_REAL_H265_SAMPLE "${STREAMVIEW_REAL_MEDIA_DIR}/video_265_sd.h265")
    if(EXISTS "${STREAMVIEW_REAL_H265_SAMPLE}")
        add_test(
            NAME streamview-cli-real-h265-smoke
            COMMAND
                ${CMAKE_COMMAND}
                -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
                -DSTREAMVIEW_SAMPLE=${STREAMVIEW_REAL_H265_SAMPLE}
                -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/real_h265_smoke.actual.json
                -DSTREAMVIEW_EXPECTED_CODEC=h265
                -DSTREAMVIEW_MIN_NAL_COUNT=100
                -DSTREAMVIEW_MIN_FRAME_COUNT=100
                -DSTREAMVIEW_REQUIRED_JSON_FIELD=active_h265_sps
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_smoke_test.cmake
        )
    endif()
endif()
