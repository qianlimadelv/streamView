if(NOT DEFINED STREAMVIEW_CLI)
    message(FATAL_ERROR "STREAMVIEW_CLI is required")
endif()
if(NOT DEFINED STREAMVIEW_SAMPLE)
    message(FATAL_ERROR "STREAMVIEW_SAMPLE is required")
endif()
if(NOT DEFINED STREAMVIEW_OUTPUT)
    message(FATAL_ERROR "STREAMVIEW_OUTPUT is required")
endif()

execute_process(
    COMMAND ${STREAMVIEW_CLI} errors ${STREAMVIEW_SAMPLE}
    RESULT_VARIABLE result
    OUTPUT_FILE ${STREAMVIEW_OUTPUT}
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "streamview errors failed: ${stderr}")
endif()

file(READ ${STREAMVIEW_OUTPUT} output_text)
string(FIND "${output_text}" "Parse errors: 0" expected_position)
if(expected_position EQUAL -1)
    message(FATAL_ERROR "Expected zero parse errors in ${STREAMVIEW_OUTPUT}")
endif()
