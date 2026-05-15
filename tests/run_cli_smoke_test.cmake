if(NOT DEFINED STREAMVIEW_CLI)
    message(FATAL_ERROR "STREAMVIEW_CLI is required")
endif()
if(NOT DEFINED STREAMVIEW_SAMPLE)
    message(FATAL_ERROR "STREAMVIEW_SAMPLE is required")
endif()
if(NOT DEFINED STREAMVIEW_OUTPUT)
    message(FATAL_ERROR "STREAMVIEW_OUTPUT is required")
endif()
if(NOT DEFINED STREAMVIEW_EXPECTED_CODEC)
    message(FATAL_ERROR "STREAMVIEW_EXPECTED_CODEC is required")
endif()
if(NOT DEFINED STREAMVIEW_MIN_NAL_COUNT)
    set(STREAMVIEW_MIN_NAL_COUNT 1)
endif()
if(NOT DEFINED STREAMVIEW_MIN_FRAME_COUNT)
    set(STREAMVIEW_MIN_FRAME_COUNT 1)
endif()

execute_process(
    COMMAND
        ${STREAMVIEW_CLI}
        analyze
        ${STREAMVIEW_SAMPLE}
        --json
        ${STREAMVIEW_OUTPUT}
    RESULT_VARIABLE command_result
    OUTPUT_VARIABLE command_stdout
    ERROR_VARIABLE command_stderr
)

if(NOT command_result EQUAL 0)
    message(FATAL_ERROR
        "CLI smoke test failed for ${STREAMVIEW_SAMPLE}\n"
        "stdout:\n${command_stdout}\n"
        "stderr:\n${command_stderr}"
    )
endif()

file(READ ${STREAMVIEW_OUTPUT} output_json)

if(NOT output_json MATCHES "\"codec_guess\": \"${STREAMVIEW_EXPECTED_CODEC}\"")
    message(FATAL_ERROR "Unexpected codec_guess for ${STREAMVIEW_SAMPLE}")
endif()

string(REGEX MATCH "\"nal_count\": ([0-9]+)" nal_count_match "${output_json}")
if(NOT nal_count_match)
    message(FATAL_ERROR "Missing nal_count in ${STREAMVIEW_OUTPUT}")
endif()
set(nal_count ${CMAKE_MATCH_1})
if(nal_count LESS STREAMVIEW_MIN_NAL_COUNT)
    message(FATAL_ERROR
        "nal_count ${nal_count} is lower than ${STREAMVIEW_MIN_NAL_COUNT} for ${STREAMVIEW_SAMPLE}"
    )
endif()

string(REGEX MATCH "\"frame_count\": ([0-9]+)" frame_count_match "${output_json}")
if(NOT frame_count_match)
    message(FATAL_ERROR "Missing frame_count in ${STREAMVIEW_OUTPUT}")
endif()
set(frame_count ${CMAKE_MATCH_1})
if(frame_count LESS STREAMVIEW_MIN_FRAME_COUNT)
    message(FATAL_ERROR
        "frame_count ${frame_count} is lower than ${STREAMVIEW_MIN_FRAME_COUNT} for ${STREAMVIEW_SAMPLE}"
    )
endif()

if(DEFINED STREAMVIEW_REQUIRED_JSON_FIELD)
    string(FIND "${output_json}" "\"${STREAMVIEW_REQUIRED_JSON_FIELD}\"" required_field_position)
    if(required_field_position EQUAL -1)
        message(FATAL_ERROR
            "Required JSON field '${STREAMVIEW_REQUIRED_JSON_FIELD}' was not found in ${STREAMVIEW_OUTPUT}"
        )
    endif()
endif()
