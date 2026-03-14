add_library(shirohachat_shared_common INTERFACE)

target_include_directories(shirohachat_shared_common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
