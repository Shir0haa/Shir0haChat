add_executable(shirohachat_jwt_gtest
    jwt_manager_gtest.cpp
    ${PROJECT_SOURCE_DIR}/server/src/auth/jwt_manager.cpp
)

target_include_directories(shirohachat_jwt_gtest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/server/src
)

target_link_libraries(shirohachat_jwt_gtest
    PRIVATE
        GTest::gtest_main
        Qt6::Core
        shirohachat_shared_common
)

gtest_discover_tests(shirohachat_jwt_gtest)
