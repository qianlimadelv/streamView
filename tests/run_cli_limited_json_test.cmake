if(NOT DEFINED STREAMVIEW_CLI)
    message(FATAL_ERROR "STREAMVIEW_CLI is required")
endif()
if(NOT DEFINED STREAMVIEW_SAMPLE)
    message(FATAL_ERROR "STREAMVIEW_SAMPLE is required")
endif()
if(NOT DEFINED STREAMVIEW_OUTPUT)
    message(FATAL_ERROR "STREAMVIEW_OUTPUT is required")
endif()
if(NOT DEFINED STREAMVIEW_LIMIT_NALS)
    message(FATAL_ERROR "STREAMVIEW_LIMIT_NALS is required")
endif()
if(NOT DEFINED STREAMVIEW_EXPECTED_OMITTED)
    message(FATAL_ERROR "STREAMVIEW_EXPECTED_OMITTED is required")
endif()

execute_process(
    COMMAND
        "${STREAMVIEW_CLI}"
        analyze
        "${STREAMVIEW_SAMPLE}"
        --json
        "${STREAMVIEW_OUTPUT}"
        --limit-nals
        "${STREAMVIEW_LIMIT_NALS}"
    RESULT_VARIABLE result
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "streamview limited JSON failed: ${stderr}")
endif()

file(READ "${STREAMVIEW_OUTPUT}" output_json)
if(NOT output_json MATCHES "\"nals_omitted\": ${STREAMVIEW_EXPECTED_OMITTED}")
    message(FATAL_ERROR "Expected nals_omitted=${STREAMVIEW_EXPECTED_OMITTED} in ${STREAMVIEW_OUTPUT}")
endif()
if(output_json MATCHES "\"index\": ${STREAMVIEW_LIMIT_NALS}")
    message(FATAL_ERROR "Limited JSON unexpectedly contains NAL index ${STREAMVIEW_LIMIT_NALS}")
endif()
