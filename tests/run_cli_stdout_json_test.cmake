if(NOT DEFINED STREAMVIEW_CLI)
    message(FATAL_ERROR "STREAMVIEW_CLI is required")
endif()
if(NOT DEFINED STREAMVIEW_SAMPLE)
    message(FATAL_ERROR "STREAMVIEW_SAMPLE is required")
endif()
if(NOT DEFINED STREAMVIEW_EXPECTED_CODEC)
    message(FATAL_ERROR "STREAMVIEW_EXPECTED_CODEC is required")
endif()

set(streamview_args analyze ${STREAMVIEW_SAMPLE} --format json --output -)
if(DEFINED STREAMVIEW_CODEC)
    list(APPEND streamview_args --codec ${STREAMVIEW_CODEC})
endif()

execute_process(
    COMMAND ${STREAMVIEW_CLI} ${streamview_args}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "streamview analyze stdout JSON failed: ${stderr}")
endif()

string(FIND "${stdout}" "\"codec_guess\": \"${STREAMVIEW_EXPECTED_CODEC}\"" codec_position)
if(codec_position EQUAL -1)
    message(FATAL_ERROR "Expected codec ${STREAMVIEW_EXPECTED_CODEC} in stdout JSON")
endif()
