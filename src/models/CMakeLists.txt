add_library(shirohachat_models STATIC
    session_list_model.h
    session_list_model.cpp
    message_list_model.h
    message_list_model.cpp
)

set_target_properties(shirohachat_models PROPERTIES AUTOMOC ON)

target_include_directories(shirohachat_models
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(shirohachat_models
    PUBLIC
        Qt6::Core
        Qt6::Qml
        shirohachat_domain
)
