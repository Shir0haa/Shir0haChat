add_library(shirohachat_client_storage STATIC
    database_manager.h database_manager.cpp
    message_repository.h message_repository.cpp
    session_repository.h session_repository.cpp
    user_repository.h user_repository.cpp
    group_repository.h group_repository.cpp
    friend_repository.h friend_repository.cpp
    auth_state_repository.h auth_state_repository.cpp
    unit_of_work.h unit_of_work.cpp
)

target_include_directories(shirohachat_client_storage
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(shirohachat_client_storage
    PUBLIC
        Qt6::Core
        Qt6::Sql
        shirohachat_shared_domain
)
