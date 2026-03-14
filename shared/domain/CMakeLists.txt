add_library(shirohachat_shared_domain STATIC
    message.h
    message.cpp
    user.h
    user.cpp
    session.h
    session.cpp
    contact.h
    contact.cpp
    group.h
    group.cpp
    friend_request.h
    friend_request.cpp
    auth_state.h
    session_summary.h
    i_message_repository.h
    i_session_repository.h
    i_user_repository.h
    i_group_repository.h
    i_friend_request_repository.h
    i_contact_repository.h
    i_auth_state_repository.h
    i_unit_of_work.h
)

set_target_properties(shirohachat_shared_domain PROPERTIES AUTOMOC ON)

target_include_directories(shirohachat_shared_domain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(shirohachat_shared_domain
    PUBLIC
        Qt6::Core
        shirohachat_shared_common
)
