if(NOT DEFINED STREAMVIEW_CLI)
    message(FATAL_ERROR "STREAMVIEW_CLI is required")
endif()
if(NOT DEFINED STREAMVIEW_SAMPLE)
    message(FATAL_ERROR "STREAMVIEW_SAMPLE is required")
endif()
if(NOT DEFINED STREAMVIEW_CASE)
    message(FATAL_ERROR "STREAMVIEW_CASE is required")
endif()
if(NOT DEFINED STREAMVIEW_EXPECTED_ERROR)
    message(FATAL_ERROR "STREAMVIEW_EXPECTED_ERROR is required")
endif()

if(STREAMVIEW_CASE STREQUAL "invalid-json-mode")
    execute_process(
        COMMAND ${STREAMVIEW_CLI} analyze ${STREAMVIEW_SAMPLE} --json-mode bad
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
    )
elseif(STREAMVIEW_CASE STREQUAL "invalid-limit-nals")
    execute_process(
        COMMAND ${STREAMVIEW_CLI} analyze ${STREAMVIEW_SAMPLE} --limit-nals abc
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
    )
else()
    message(FATAL_ERROR "Unknown invalid args test case: ${STREAMVIEW_CASE}")
endif()

if(result EQUAL 0)
    message(FATAL_ERROR "Expected CLI command to fail, but it succeeded")
endif()

string(FIND "${stderr}" "${STREAMVIEW_EXPECTED_ERROR}" expected_position)
if(expected_position EQUAL -1)
    message(FATAL_ERROR
        "Expected error '${STREAMVIEW_EXPECTED_ERROR}' was not found.\n"
        "stdout:\n${stdout}\n"
        "stderr:\n${stderr}"
    )
endif()
