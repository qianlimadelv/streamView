add_library(sv-core
    src/status.cpp
)

add_library(StreamView::Core ALIAS sv-core)

target_include_directories(sv-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(sv-core PUBLIC cxx_std_20)
