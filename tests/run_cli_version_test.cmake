if(NOT DEFINED STREAMVIEW_CLI)
    message(FATAL_ERROR "STREAMVIEW_CLI is required")
endif()

execute_process(
    COMMAND ${STREAMVIEW_CLI} --version
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "streamview --version failed: ${stderr}")
endif()

if(NOT stdout MATCHES "^streamview [0-9]+\\.[0-9]+\\.[0-9]+")
    message(FATAL_ERROR "Unexpected version output: ${stdout}")
endif()
