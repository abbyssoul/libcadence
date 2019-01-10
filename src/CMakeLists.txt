
set(SOURCE_FILES
        asyncServer.cpp
        ipendpoint.cpp
        ioobject.cpp
        version.cpp

        io/selector_epoll.cpp
        io/duplex.cpp
        io/selector_poll.cpp
        io/platformFilesystem.cpp
        io/sharedMemory.cpp
        io/file.cpp
        io/signalDispatcher.cpp
        io/serial.cpp
        io/selector.cpp
        io/serial_list_ports.cpp
        io/pipe.cpp

        async/streamsocket.cpp
        async/eventLoop.cpp
        async/udpsocket.cpp
        async/tcpsocket.cpp
        async/event.cpp
        async/acceptor.cpp
        async/serialChannel.cpp
        async/async.cpp
        async/pipe.cpp
        async/streamdomainsocket.cpp
        async/timer.cpp
        async/streamdomainacceptor.cpp
        async/datagramdomainsocket.cpp
        async/signalSet.cpp
        async/tcpacceptor.cpp
        )

add_library(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${CONAN_LIBS})
#set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FEATURES "-DASIO_STANDALONE")
add_definitions("-DASIO_STANDALONE")

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ../${CADENCE_EXTERNAL_DEP_ASIO_DIR})


install(TARGETS ${PROJECT_NAME}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
