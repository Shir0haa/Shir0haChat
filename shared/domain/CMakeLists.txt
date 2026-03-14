add_library(shirohachat_shared_domain INTERFACE)

target_include_directories(shirohachat_shared_domain
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(shirohachat_shared_domain
    INTERFACE
        Qt6::Core
        shirohachat_shared_common
)
