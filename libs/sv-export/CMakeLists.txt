add_library(sv-export
    src/json_writer.cpp
)

add_library(StreamView::Export ALIAS sv-export)

target_include_directories(sv-export
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(sv-export PUBLIC cxx_std_20)
