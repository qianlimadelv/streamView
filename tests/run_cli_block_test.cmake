if(NOT DEFINED STREAMVIEW_CLI)
    message(FATAL_ERROR "STREAMVIEW_CLI is required")
endif()
if(NOT DEFINED STREAMVIEW_SAMPLE)
    message(FATAL_ERROR "STREAMVIEW_SAMPLE is required")
endif()
if(NOT DEFINED STREAMVIEW_OUTPUT)
    message(FATAL_ERROR "STREAMVIEW_OUTPUT is required")
endif()
if(NOT DEFINED STREAMVIEW_LAYER)
    set(STREAMVIEW_LAYER qp)
endif()
if(NOT DEFINED STREAMVIEW_FRAME)
    set(STREAMVIEW_FRAME 0)
endif()

execute_process(
    COMMAND ${STREAMVIEW_CLI} decode ${STREAMVIEW_SAMPLE} --frame ${STREAMVIEW_FRAME}
            --block-layer ${STREAMVIEW_LAYER} --block-out ${STREAMVIEW_OUTPUT} --thumb-size 160
    RESULT_VARIABLE result
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "streamview decode --block-layer ${STREAMVIEW_LAYER} failed: ${stderr}")
endif()

# The overlay must be a valid binary P6 PPM ("P6" == hex 5036).
file(READ ${STREAMVIEW_OUTPUT} head LIMIT 2 HEX)
if(NOT head STREQUAL "5036")
    message(FATAL_ERROR "Expected a P6 PPM overlay in ${STREAMVIEW_OUTPUT} (got 0x${head})")
endif()
