if(NOT DEFINED STREAMVIEW_CLI)
    message(FATAL_ERROR "STREAMVIEW_CLI is required")
endif()

if(NOT DEFINED STREAMVIEW_SAMPLE)
    message(FATAL_ERROR "STREAMVIEW_SAMPLE is required")
endif()

if(NOT DEFINED STREAMVIEW_GOLDEN)
    message(FATAL_ERROR "STREAMVIEW_GOLDEN is required")
endif()

if(NOT DEFINED STREAMVIEW_OUTPUT)
    message(FATAL_ERROR "STREAMVIEW_OUTPUT is required")
endif()

execute_process(
    COMMAND "${STREAMVIEW_CLI}" analyze "${STREAMVIEW_SAMPLE}" --json "${STREAMVIEW_OUTPUT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "streamview analyze failed: ${stderr}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DACTUAL=${STREAMVIEW_OUTPUT}"
        "-DEXPECTED=${STREAMVIEW_GOLDEN}"
        -P "${CMAKE_CURRENT_LIST_DIR}/compare_text_files.cmake"
    RESULT_VARIABLE compare_result
)

if(NOT compare_result EQUAL 0)
    message(FATAL_ERROR "CLI JSON output differs from golden file: ${STREAMVIEW_OUTPUT} != ${STREAMVIEW_GOLDEN}")
endif()
