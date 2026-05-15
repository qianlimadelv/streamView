if(NOT DEFINED STREAMVIEW_CLI)
    message(FATAL_ERROR "STREAMVIEW_CLI is required")
endif()
if(NOT DEFINED STREAMVIEW_SAMPLE)
    message(FATAL_ERROR "STREAMVIEW_SAMPLE is required")
endif()
if(NOT DEFINED STREAMVIEW_OUTPUT)
    message(FATAL_ERROR "STREAMVIEW_OUTPUT is required")
endif()

set(streamview_errors_args errors ${STREAMVIEW_SAMPLE})
if(DEFINED STREAMVIEW_ERRORS_JSON AND STREAMVIEW_ERRORS_JSON)
    list(APPEND streamview_errors_args --json)
endif()

execute_process(
    COMMAND ${STREAMVIEW_CLI} ${streamview_errors_args}
    RESULT_VARIABLE result
    OUTPUT_FILE ${STREAMVIEW_OUTPUT}
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "streamview errors failed: ${stderr}")
endif()

file(READ ${STREAMVIEW_OUTPUT} output_text)
if(DEFINED STREAMVIEW_EXPECTED_PATTERN)
    set(expected_pattern "${STREAMVIEW_EXPECTED_PATTERN}")
elseif(DEFINED STREAMVIEW_ERRORS_JSON AND STREAMVIEW_ERRORS_JSON)
    set(expected_pattern "\"parse_error_count\": 0")
else()
    set(expected_pattern "Parse errors: 0")
endif()

string(FIND "${output_text}" "${expected_pattern}" expected_position)
if(expected_position EQUAL -1)
    message(FATAL_ERROR "Expected pattern ${expected_pattern} in ${STREAMVIEW_OUTPUT}")
endif()
