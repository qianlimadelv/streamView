if(NOT DEFINED STREAMVIEW_CLI)
    message(FATAL_ERROR "STREAMVIEW_CLI is required")
endif()
if(NOT DEFINED STREAMVIEW_SAMPLE)
    message(FATAL_ERROR "STREAMVIEW_SAMPLE is required")
endif()
if(NOT DEFINED STREAMVIEW_NAL_INDEX)
    message(FATAL_ERROR "STREAMVIEW_NAL_INDEX is required")
endif()
if(NOT DEFINED STREAMVIEW_OUTPUT)
    message(FATAL_ERROR "STREAMVIEW_OUTPUT is required")
endif()
if(NOT DEFINED STREAMVIEW_REQUIRED_FIELD)
    message(FATAL_ERROR "STREAMVIEW_REQUIRED_FIELD is required")
endif()

if(NOT DEFINED STREAMVIEW_INSPECT_KIND)
    set(STREAMVIEW_INSPECT_KIND nal)
endif()

if(STREAMVIEW_INSPECT_KIND STREQUAL "nal")
    set(selector --nal)
elseif(STREAMVIEW_INSPECT_KIND STREQUAL "frame")
    set(selector --frame)
elseif(STREAMVIEW_INSPECT_KIND STREQUAL "gop")
    set(selector --gop)
else()
    message(FATAL_ERROR "Unknown inspect kind: ${STREAMVIEW_INSPECT_KIND}")
endif()

execute_process(
    COMMAND ${STREAMVIEW_CLI} inspect ${STREAMVIEW_SAMPLE} ${selector} ${STREAMVIEW_NAL_INDEX}
    RESULT_VARIABLE result
    OUTPUT_FILE ${STREAMVIEW_OUTPUT}
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "streamview inspect failed: ${stderr}")
endif()

file(READ ${STREAMVIEW_OUTPUT} output_json)
string(FIND "${output_json}" "\"${STREAMVIEW_REQUIRED_FIELD}\"" required_position)
if(required_position EQUAL -1)
    message(FATAL_ERROR "Required field ${STREAMVIEW_REQUIRED_FIELD} was not found in inspect output")
endif()
if(NOT output_json MATCHES "\"index\": ${STREAMVIEW_NAL_INDEX}")
    message(FATAL_ERROR "Expected inspect output for NAL index ${STREAMVIEW_NAL_INDEX}")
endif()
