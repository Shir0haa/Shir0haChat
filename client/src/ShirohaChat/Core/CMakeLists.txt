add_library(shirohachat_core STATIC
    message_list_model.h message_list_model.cpp
    session_list_model.h session_list_model.cpp
)

set_target_properties(shirohachat_core PROPERTIES AUTOMOC ON)

target_include_directories(shirohachat_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(shirohachat_core
    PUBLIC Qt6::Core Qt6::Qml shirohachat_shared_domain
)
