add_executable(shirohachat-server
    src/main.cpp
    src/server_app.h src/server_app.cpp
    src/client_session.h src/client_session.cpp
    src/services/session_manager.h src/services/session_manager.cpp
    src/storage/server_db.h src/storage/server_db.cpp
    src/handlers/heartbeat_handler.h src/handlers/heartbeat_handler.cpp
    src/handlers/i_command_handler.h
    src/dispatch/command_dispatcher.h src/dispatch/command_dispatcher.cpp
    src/auth/jwt_manager.h src/auth/jwt_manager.cpp
    src/handlers/auth_handler.h src/handlers/auth_handler.cpp
    src/handlers/group_handler.h src/handlers/group_handler.cpp
    src/handlers/friend_handler.h src/handlers/friend_handler.cpp
    src/application/services/message_delivery_service.h src/application/services/message_delivery_service.cpp
    src/application/services/friend_management_service.h src/application/services/friend_management_service.cpp
    src/application/services/group_management_service.h src/application/services/group_management_service.cpp
    src/handlers/message_handler.h src/handlers/message_handler.cpp
    src/storage/message_store.h src/storage/message_store.cpp
    src/storage/message_repository.h src/storage/message_repository.cpp
    src/storage/offline_queue_repository.h src/storage/offline_queue_repository.cpp
    src/storage/group_repository.h src/storage/group_repository.cpp
    src/storage/friend_repository.h src/storage/friend_repository.cpp
    src/services/message_router.h src/services/message_router.cpp
    src/services/offline_manager.h src/services/offline_manager.cpp
)

target_include_directories(shirohachat-server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/application
)

target_link_libraries(shirohachat-server
    PRIVATE
        Qt6::Core
        Qt6::WebSockets
        Qt6::Sql
        shirohachat_shared_domain
        shirohachat_shared_protocol
        shirohachat_shared_common
)
