add_executable(shirohachat_packet_gtest
    packet_gtest.cpp
)

target_link_libraries(shirohachat_packet_gtest
    PRIVATE
        GTest::gtest_main
        shirohachat_shared_protocol
)

gtest_discover_tests(shirohachat_packet_gtest)
