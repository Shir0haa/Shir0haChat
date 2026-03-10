add_library(shirohachat_domain STATIC
    message.h
    message.cpp
    user.h
    user.cpp
    session.h
    session.cpp
)

set_target_properties(shirohachat_domain PROPERTIES AUTOMOC ON)

target_include_directories(shirohachat_domain
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(shirohachat_domain
    PUBLIC Qt6::Core
)
