if(NOT DEFINED STREAMVIEW_CLI)
    message(FATAL_ERROR "STREAMVIEW_CLI is required")
endif()
if(NOT DEFINED STREAMVIEW_SAMPLE)
    message(FATAL_ERROR "STREAMVIEW_SAMPLE is required")
endif()
if(NOT DEFINED STREAMVIEW_OUTPUT)
    message(FATAL_ERROR "STREAMVIEW_OUTPUT is required")
endif()

set(streamview_validate_args validate ${STREAMVIEW_SAMPLE})
if(DEFINED STREAMVIEW_VALIDATE_JSON AND STREAMVIEW_VALIDATE_JSON)
    list(APPEND streamview_validate_args --json)
endif()

execute_process(
    COMMAND ${STREAMVIEW_CLI} ${streamview_validate_args}
    RESULT_VARIABLE result
    OUTPUT_FILE ${STREAMVIEW_OUTPUT}
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "streamview validate failed: ${stderr}")
endif()

file(READ ${STREAMVIEW_OUTPUT} output_text)
if(DEFINED STREAMVIEW_EXPECTED_PATTERN)
    set(expected_pattern "${STREAMVIEW_EXPECTED_PATTERN}")
elseif(DEFINED STREAMVIEW_VALIDATE_JSON AND STREAMVIEW_VALIDATE_JSON)
    set(expected_pattern "\"issue_count\": 0")
else()
    set(expected_pattern "Validation issues: 0")
endif()

string(FIND "${output_text}" "${expected_pattern}" expected_position)
if(expected_position EQUAL -1)
    message(FATAL_ERROR "Expected pattern ${expected_pattern} in ${STREAMVIEW_OUTPUT}")
endif()
