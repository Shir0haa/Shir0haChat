add_library(shirohachat_shared_protocol INTERFACE)

target_include_directories(shirohachat_shared_protocol
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(shirohachat_shared_protocol
    INTERFACE
        Qt6::Core
        shirohachat_shared_domain
        shirohachat_shared_common
)
