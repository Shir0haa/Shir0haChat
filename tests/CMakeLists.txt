include(GoogleTest)

find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
    find_package(GTest QUIET)
endif()

if(NOT GTest_FOUND)
    message(FATAL_ERROR
        "BUILD_TESTING=ON requires GoogleTest. Install GTest on this machine and reconfigure the project.")
endif()

if(TARGET shirohachat_shared_protocol)
    add_subdirectory(shared)
endif()

if(TARGET shirohachat-server)
    add_subdirectory(server)
endif()

if(TARGET shirohachat_core AND TARGET shirohachat_client_storage)
    add_subdirectory(client)
endif()
