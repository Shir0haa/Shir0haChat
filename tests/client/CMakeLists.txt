add_executable(shirohachat_message_list_model_qtest
    message_list_model_qtest.cpp
)

target_link_libraries(shirohachat_message_list_model_qtest
    PRIVATE
        Qt6::Test
        shirohachat_core
)

add_test(NAME shirohachat_message_list_model_qtest
    COMMAND shirohachat_message_list_model_qtest
)

add_executable(shirohachat_session_list_model_qtest
    session_list_model_qtest.cpp
)

target_link_libraries(shirohachat_session_list_model_qtest
    PRIVATE
        Qt6::Test
        shirohachat_core
)

add_test(NAME shirohachat_session_list_model_qtest
    COMMAND shirohachat_session_list_model_qtest
)

add_executable(shirohachat_session_repository_qtest
    session_repository_qtest.cpp
)

target_link_libraries(shirohachat_session_repository_qtest
    PRIVATE
        Qt6::Test
        shirohachat_client_storage
)

add_test(NAME shirohachat_session_repository_qtest
    COMMAND shirohachat_session_repository_qtest
)

add_executable(shirohachat_network_service_qtest
    network_service_qtest.cpp
    ${PROJECT_SOURCE_DIR}/client/src/ShirohaChat/network_service.cpp
    ${PROJECT_SOURCE_DIR}/client/src/ShirohaChat/Application/Ports/INetworkGateway.h
    ${PROJECT_SOURCE_DIR}/client/src/ShirohaChat/Application/Contracts/group_list_entry_dto.h
    ${PROJECT_SOURCE_DIR}/client/src/ShirohaChat/Application/Contracts/friend_request_dto.h
)

set_target_properties(shirohachat_network_service_qtest PROPERTIES AUTOMOC ON)

target_include_directories(shirohachat_network_service_qtest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/client/src/ShirohaChat
)

target_link_libraries(shirohachat_network_service_qtest
    PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Test
        Qt6::WebSockets
        shirohachat_shared_common
        shirohachat_shared_domain
        shirohachat_shared_protocol
)

add_test(NAME shirohachat_network_service_qtest
    COMMAND shirohachat_network_service_qtest
)
