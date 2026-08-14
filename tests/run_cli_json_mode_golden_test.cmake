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
if(NOT DEFINED STREAMVIEW_JSON_MODE)
    message(FATAL_ERROR "STREAMVIEW_JSON_MODE is required")
endif()

execute_process(
    COMMAND
        "${STREAMVIEW_CLI}"
        analyze
        "${STREAMVIEW_SAMPLE}"
        --json
        "${STREAMVIEW_OUTPUT}"
        --json-mode
        "${STREAMVIEW_JSON_MODE}"
    RESULT_VARIABLE result
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "streamview analyze failed: ${stderr}")
endif()

if(STREAMVIEW_NORMALIZE_INPUT_PATH)
    get_filename_component(normalized_input "${STREAMVIEW_SAMPLE}" NAME)
    file(READ "${STREAMVIEW_OUTPUT}" actual_json)
    string(REGEX REPLACE
        "(\"input\"[ \\t]*:[ \\t]*\")[^\"]*(\")"
        "\\1${normalized_input}\\2"
        normalized_json
        "${actual_json}"
    )
    file(WRITE "${STREAMVIEW_OUTPUT}" "${normalized_json}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${STREAMVIEW_OUTPUT}" "${STREAMVIEW_GOLDEN}"
    RESULT_VARIABLE compare_result
)

if(NOT compare_result EQUAL 0)
    message(FATAL_ERROR "CLI JSON mode output differs from golden file: ${STREAMVIEW_OUTPUT} != ${STREAMVIEW_GOLDEN}")
endif()
