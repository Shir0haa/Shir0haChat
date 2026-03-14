add_subdirectory(Storage)

qt_add_executable(shirohachat_client main.cpp)

qt_add_qml_module(shirohachat_client
    URI ShirohaChat
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES
        app_coordinator.h app_coordinator.cpp
        network_service.h network_service.cpp
        Controllers/auth_controller.h Controllers/auth_controller.cpp
        Controllers/session_list_controller.h Controllers/session_list_controller.cpp
        Controllers/chat_view_model.h Controllers/chat_view_model.cpp
        Controllers/contact_controller.h Controllers/contact_controller.cpp
        Controllers/group_controller.h Controllers/group_controller.cpp
        UseCases/group_management_use_case.h UseCases/group_management_use_case.cpp
        UseCases/authenticate_use_case.h UseCases/authenticate_use_case.cpp
        UseCases/friend_request_use_case.h UseCases/friend_request_use_case.cpp
        UseCases/send_message_use_case.h UseCases/send_message_use_case.cpp
        UseCases/receive_message_use_case.h UseCases/receive_message_use_case.cpp
        UseCases/session_sync_use_case.h UseCases/session_sync_use_case.cpp
        UseCases/switch_session_use_case.h UseCases/switch_session_use_case.cpp
        Application/Contracts/use_case_errors.h
        Application/Contracts/session_list_item_dto.h
        Application/Contracts/group_session_info_dto.h
        Application/Contracts/friend_request_dto.h
        Application/Contracts/friend_list_item_dto.h
        Application/Contracts/send_message_result_dto.h
        Application/Contracts/group_action_result_dto.h
        Application/Contracts/authenticate_result_dto.h
        Application/Contracts/group_list_entry_dto.h
        Application/Contracts/group_member_dto.h
        Application/Contracts/session_summary_dto.h
        Application/Queries/i_group_queries.h
        Application/Queries/i_friend_queries.h
        Application/Queries/i_session_queries.h
        Application/Ports/INetworkGateway.h
        Application/Services/CurrentSessionService.h
    DEPENDENCIES
        ShirohaChat.UI.Views
        ShirohaChat.UI.Components
)

target_link_libraries(shirohachat_client
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::WebSockets
        Qt6::Sql
        shirohachat_shared_domain
        shirohachat_shared_protocol
        shirohachat_shared_common
        shirohachat_core
        shirohachat_client_storage
        shirohachat_ui_views
        shirohachat_ui_components
)

target_include_directories(shirohachat_client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Controllers
)
