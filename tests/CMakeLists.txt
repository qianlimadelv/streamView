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
    NAME streamview-cli-version
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_version_test.cmake
)

add_test(
    NAME streamview-cli-invalid-json-mode
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_CASE=invalid-json-mode
        -DSTREAMVIEW_EXPECTED_ERROR=--json-mode\ must\ be\ full\ or\ summary
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_invalid_args_test.cmake
)

set_tests_properties(streamview-cli-invalid-json-mode
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-invalid-limit-nals
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_CASE=invalid-limit-nals
        -DSTREAMVIEW_EXPECTED_ERROR=--limit-nals\ requires\ a\ non-negative\ integer
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_invalid_args_test.cmake
)

set_tests_properties(streamview-cli-invalid-limit-nals
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-invalid-format
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_CASE=invalid-format
        -DSTREAMVIEW_EXPECTED_ERROR=--format\ must\ be\ text,\ json,\ or\ csv
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_invalid_args_test.cmake
)

set_tests_properties(streamview-cli-invalid-format
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-csv-h264-minimal
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/h264_minimal_summary.csv
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_minimal_summary.actual.csv
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_csv_golden_test.cmake
)

set_tests_properties(streamview-cli-csv-h264-minimal
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-csv-h265-minimal
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h265/minimal_vps_sps_pps_idr.h265
        -DSTREAMVIEW_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/h265_minimal_summary.csv
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h265_minimal_summary.actual.csv
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_csv_golden_test.cmake
)

set_tests_properties(streamview-cli-csv-h265-minimal
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-invalid-codec
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_CASE=invalid-codec
        -DSTREAMVIEW_EXPECTED_ERROR=--codec\ must\ be\ auto,\ h264,\ or\ h265
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_invalid_args_test.cmake
)

set_tests_properties(streamview-cli-invalid-codec
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-invalid-dump-format
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_CASE=invalid-dump-format
        -DSTREAMVIEW_EXPECTED_ERROR=--format\ must\ be\ hex,\ payload,\ or\ rbsp
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_invalid_args_test.cmake
)

set_tests_properties(streamview-cli-invalid-dump-format
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

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
    NAME streamview-cli-json-summary-mp4-h264
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/mp4/minimal_h264.mp4
        -DSTREAMVIEW_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/minimal_h264_mp4_summary.json
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/minimal_h264_mp4_summary.actual.json
        -DSTREAMVIEW_JSON_MODE=summary
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_json_mode_golden_test.cmake
)

set_tests_properties(streamview-cli-json-summary-mp4-h264
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
    NAME streamview-cli-inspect-h264-sps
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_NAL_INDEX=0
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_inspect_sps.actual.json
        -DSTREAMVIEW_REQUIRED_FIELD=width
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_inspect_test.cmake
)

set_tests_properties(streamview-cli-inspect-h264-sps
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-inspect-h264-frame
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_NAL_INDEX=0
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_inspect_frame.actual.json
        -DSTREAMVIEW_REQUIRED_FIELD=frame_type
        -DSTREAMVIEW_EXPECTED_FRAME_FIELD=poc
        -DSTREAMVIEW_INSPECT_KIND=frame
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_inspect_test.cmake
)

set_tests_properties(streamview-cli-inspect-h264-frame
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-inspect-h264-gop
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_NAL_INDEX=0
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_inspect_gop.actual.json
        -DSTREAMVIEW_REQUIRED_FIELD=start_frame_index
        -DSTREAMVIEW_INSPECT_KIND=gop
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_inspect_test.cmake
)

set_tests_properties(streamview-cli-inspect-h264-gop
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-dump-h264-sps-hex
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_NAL_INDEX=0
        -DSTREAMVIEW_DUMP_FORMAT=hex
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_dump_sps.actual.hex
        -DSTREAMVIEW_EXPECTED_HEX=67
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_dump_test.cmake
)

set_tests_properties(streamview-cli-dump-h264-sps-hex
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-dump-h264-sps-payload
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_NAL_INDEX=0
        -DSTREAMVIEW_DUMP_FORMAT=payload
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_dump_sps.actual.bin
        -DSTREAMVIEW_EXPECTED_MIN_SIZE=4
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_dump_test.cmake
)

set_tests_properties(streamview-cli-dump-h264-sps-payload
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-errors-h264-clean
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_errors.actual.txt
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_errors_test.cmake
)

set_tests_properties(streamview-cli-errors-h264-clean
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-errors-h264-json-clean
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_errors.actual.json
        -DSTREAMVIEW_ERRORS_JSON=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_errors_test.cmake
)

set_tests_properties(streamview-cli-errors-h264-json-clean
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-validate-h264-clean
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_validate.actual.txt
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_validate_test.cmake
)

set_tests_properties(streamview-cli-validate-h264-clean
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-validate-h264-json-clean
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/h264_validate.actual.json
        -DSTREAMVIEW_VALIDATE_JSON=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_validate_test.cmake
)

set_tests_properties(streamview-cli-validate-h264-json-clean
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-json-stdout-h264-minimal
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h264/minimal_sps_pps_idr.h264
        -DSTREAMVIEW_EXPECTED_CODEC=h264
        -DSTREAMVIEW_CODEC=h264
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_stdout_json_test.cmake
)

set_tests_properties(streamview-cli-json-stdout-h264-minimal
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
    NAME streamview-cli-json-full-mp4-h264
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/mp4/minimal_h264.mp4
        -DSTREAMVIEW_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/minimal_h264_mp4_full.json
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/minimal_h264_mp4_full.actual.json
        -DSTREAMVIEW_JSON_MODE=full
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_json_mode_golden_test.cmake
)

set_tests_properties(streamview-cli-json-full-mp4-h264
    PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME streamview-cli-json-summary-mp4-h265
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/mp4/short_h265.mp4
        -DSTREAMVIEW_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/short_h265_mp4_summary.json
        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/short_h265_mp4_summary.actual.json
        -DSTREAMVIEW_JSON_MODE=summary
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_json_mode_golden_test.cmake
)

set_tests_properties(streamview-cli-json-summary-mp4-h265
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

add_test(
    NAME streamview-cli-json-stdout-h265-minimal
    COMMAND
        ${CMAKE_COMMAND}
        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
        -DSTREAMVIEW_SAMPLE=tests/samples/h265/minimal_vps_sps_pps_idr.h265
        -DSTREAMVIEW_EXPECTED_CODEC=h265
        -DSTREAMVIEW_CODEC=h265
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_stdout_json_test.cmake
)

set_tests_properties(streamview-cli-json-stdout-h265-minimal
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
        if(STREAMVIEW_REAL_MEDIA_DIR STREQUAL "/home/zhangyp/code/media")
            add_test(
                NAME streamview-cli-real-h264-summary-golden
                COMMAND
                    ${CMAKE_COMMAND}
                    -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
                    -DSTREAMVIEW_SAMPLE=${STREAMVIEW_REAL_H264_SAMPLE}
                    -DSTREAMVIEW_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/real_h264_video_264_hd_summary.json
                    -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/real_h264_summary.actual.json
                    -DSTREAMVIEW_JSON_MODE=summary
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_json_mode_golden_test.cmake
            )
        endif()
        # Decode smoke test needs the FFmpeg decode module (real, decodable input).
        if(STREAMVIEW_HAS_FFMPEG_DEMUX)
            add_test(
                NAME streamview-cli-real-h264-decode
                COMMAND
                    ${CMAKE_COMMAND}
                    -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
                    -DSTREAMVIEW_SAMPLE=${STREAMVIEW_REAL_H264_SAMPLE}
                    -DSTREAMVIEW_THUMB=${CMAKE_CURRENT_BINARY_DIR}/real_h264_frame0.ppm
                    -DSTREAMVIEW_MV_JSON=${CMAKE_CURRENT_BINARY_DIR}/real_h264_frame0_mv.json
                    -DSTREAMVIEW_FRAME=0
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_decode_test.cmake
            )
        endif()
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
        if(STREAMVIEW_REAL_MEDIA_DIR STREQUAL "/home/zhangyp/code/media")
            add_test(
                NAME streamview-cli-real-h265-summary-golden
                COMMAND
                    ${CMAKE_COMMAND}
                    -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
                    -DSTREAMVIEW_SAMPLE=${STREAMVIEW_REAL_H265_SAMPLE}
                    -DSTREAMVIEW_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/real_h265_video_265_sd_summary.json
                    -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/real_h265_summary.actual.json
                    -DSTREAMVIEW_JSON_MODE=summary
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_json_mode_golden_test.cmake
            )
        endif()
        # HEVC block-level overlay smoke tests (need libde265).
        if(LIBDE265_FOUND)
            foreach(layer qp partition intra)
                add_test(
                    NAME streamview-cli-real-h265-block-${layer}
                    COMMAND
                        ${CMAKE_COMMAND}
                        -DSTREAMVIEW_CLI=$<TARGET_FILE:streamview>
                        -DSTREAMVIEW_SAMPLE=${STREAMVIEW_REAL_H265_SAMPLE}
                        -DSTREAMVIEW_LAYER=${layer}
                        -DSTREAMVIEW_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/real_h265_block_${layer}.ppm
                        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_block_test.cmake
                )
            endforeach()
        endif()
    endif()
endif()
