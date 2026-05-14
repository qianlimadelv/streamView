cmake_minimum_required(VERSION 3.24)

project(StreamView
    VERSION 0.1.0
    DESCRIPTION "Cross-platform compressed video bitstream analysis prototype"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(STREAMVIEW_BUILD_TESTS "Build StreamView tests" ON)

add_subdirectory(libs/sv-core)
add_subdirectory(libs/sv-bitstream)
add_subdirectory(libs/sv-analysis)
add_subdirectory(libs/sv-export)
add_subdirectory(apps/streamview-cli)

if(STREAMVIEW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
