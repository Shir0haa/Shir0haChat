add_library(shirohachat_shared_protocol STATIC
    packet.h
    packet.cpp
    commands.h
    commands.cpp
    protocol_version.h
)

set_target_properties(shirohachat_shared_protocol PROPERTIES AUTOMOC ON)

target_include_directories(shirohachat_shared_protocol
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(shirohachat_shared_protocol
    PUBLIC
        Qt6::Core
        shirohachat_shared_domain
        shirohachat_shared_common
)
