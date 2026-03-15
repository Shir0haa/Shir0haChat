include(GoogleTest)

find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
    find_package(GTest QUIET)
endif()

if(NOT GTest_FOUND)
    message(FATAL_ERROR
        "BUILD_TESTING=ON requires GoogleTest. Install GTest on this machine and reconfigure the project.")
endif()

add_subdirectory(shared)
add_subdirectory(server)
add_subdirectory(client)
