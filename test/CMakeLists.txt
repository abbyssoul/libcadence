# TODO(abbyssoul): Disabled as not supported by clang (without gold linker that is)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")

set(TEST_SOURCE_FILES
        main_gtest.cpp
        ci/teamcity_messages.cpp
        ci/teamcity_gtest.cpp

        test_ipaddress.cpp

        io/test_mappedMemory.cpp
        io/test_platformfilesystem.cpp
        io/test_selector_poll.cpp
        io/test_selector_epoll.cpp
        io/test_sharedMemory.cpp
        io/test_signalDispatcher.cpp
        io/test_pipe.cpp

        async/test_tcpsocket.cpp
        async/test_streamdomainsocket.cpp
        async/test_signalSet.cpp
        async/test_event.cpp
        async/test_datagramdomainsocket.cpp
        async/test_timer.cpp
        async/test_udpsocket.cpp
        async/test_pipe.cpp
        )


if(CADENCE_GTEST_SUPPORT)
    message(STATUS, "Using local version of GTest")

    add_subdirectory(../${CADENCE_EXTERNAL_DEP_GTEST_DIR} ${CMAKE_BINARY_DIR}/gtest EXCLUDE_FROM_ALL)
    include_directories(SYSTEM ${gtest_SOURCE_DIR}/include)
else()
    message(STATUS, "Using provided version of GTest")
endif()

enable_testing()
add_executable(test_${PROJECT_NAME} EXCLUDE_FROM_ALL ${TEST_SOURCE_FILES})
add_test(NAME test_${PROJECT_NAME}
    COMMAND test_${PROJECT_NAME}
    )

target_link_libraries(test_${PROJECT_NAME} PRIVATE
    ${PROJECT_NAME}
    gtest
    $<$<NOT:$<PLATFORM_ID:Darwin>>:rt>
    )
