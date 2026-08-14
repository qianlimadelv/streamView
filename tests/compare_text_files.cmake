if(NOT DEFINED ACTUAL)
    message(FATAL_ERROR "ACTUAL is required")
endif()

if(NOT DEFINED EXPECTED)
    message(FATAL_ERROR "EXPECTED is required")
endif()

if(NOT EXISTS "${ACTUAL}")
    message(FATAL_ERROR "Actual output file does not exist: ${ACTUAL}")
endif()

if(NOT EXISTS "${EXPECTED}")
    message(FATAL_ERROR "Golden file does not exist: ${EXPECTED}")
endif()

file(READ "${ACTUAL}" actual_text)
file(READ "${EXPECTED}" expected_text)

string(REPLACE "\r\n" "\n" actual_normalized "${actual_text}")
string(REPLACE "\r" "\n" actual_normalized "${actual_normalized}")
string(REPLACE "\r\n" "\n" expected_normalized "${expected_text}")
string(REPLACE "\r" "\n" expected_normalized "${expected_normalized}")

if(NOT actual_normalized STREQUAL expected_normalized)
    message(FATAL_ERROR "Text output differs from golden file: ${ACTUAL} != ${EXPECTED}")
endif()
