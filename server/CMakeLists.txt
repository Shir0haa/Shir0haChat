add_executable(shirohachat-server
    src/main.cpp
    src/server_app.h src/server_app.cpp
)

target_include_directories(shirohachat-server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
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
