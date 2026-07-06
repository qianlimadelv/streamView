if(NOT DEFINED STREAMVIEW_CLI)
    message(FATAL_ERROR "STREAMVIEW_CLI is required")
endif()
if(NOT DEFINED STREAMVIEW_SAMPLE)
    message(FATAL_ERROR "STREAMVIEW_SAMPLE is required")
endif()
if(NOT DEFINED STREAMVIEW_THUMB)
    message(FATAL_ERROR "STREAMVIEW_THUMB is required")
endif()
if(NOT DEFINED STREAMVIEW_MV_JSON)
    message(FATAL_ERROR "STREAMVIEW_MV_JSON is required")
endif()
if(NOT DEFINED STREAMVIEW_FRAME)
    set(STREAMVIEW_FRAME 0)
endif()

execute_process(
    COMMAND ${STREAMVIEW_CLI} decode ${STREAMVIEW_SAMPLE} --frame ${STREAMVIEW_FRAME}
            --thumb ${STREAMVIEW_THUMB} --thumb-size 160 --mv-json ${STREAMVIEW_MV_JSON}
    RESULT_VARIABLE result
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "streamview decode failed: ${stderr}")
endif()

# The thumbnail must be a valid binary P6 PPM ("P6" == hex 5036).
file(READ ${STREAMVIEW_THUMB} thumb_head LIMIT 2 HEX)
if(NOT thumb_head STREQUAL "5036")
    message(FATAL_ERROR "Expected a P6 PPM thumbnail in ${STREAMVIEW_THUMB} (got 0x${thumb_head})")
endif()

# The MV JSON must carry the coded dimensions and picture type.
file(READ ${STREAMVIEW_MV_JSON} mv_text)
foreach(field coded_width coded_height pict_type motion_vector_count)
    string(FIND "${mv_text}" "\"${field}\"" pos)
    if(pos EQUAL -1)
        message(FATAL_ERROR "Expected field ${field} in ${STREAMVIEW_MV_JSON}")
    endif()
endforeach()
