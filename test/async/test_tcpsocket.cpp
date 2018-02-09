/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence Unit Test Suit
 * @file: test/async/test_tcpsocket.cpp
 * @author: soultaker
 *
 * Created on: 10/10/2016
 *******************************************************************************/
#include <cadence/async/tcpsocket.hpp>  // Class being tested

#include "gtest/gtest.h"

#include <thread>
#include <unistd.h>
#include <fcntl.h>

using namespace Solace;
using namespace cadence;
using namespace cadence::async;


TEST(TestTcpSocket, testAsyncConnect) {
    EventLoop iocontext;

    TcpSocket serverSocket(iocontext);
    bool connectionEstablished = false;
    bool connectionAccepted = false;

    TcpAcceptor acceptor(iocontext, 0);
    acceptor.asyncAccept(serverSocket)
            .then([&connectionAccepted]() {
                connectionAccepted = true;
            }).onError([&connectionAccepted](Error&& e) {
                connectionAccepted = false;
                FAIL() << e.toString();
            });

    const IPEndpoint endpoint {"127.0.0.1", acceptor.getLocalEndpoint().getPort()};
    TcpSocket clientSocket(iocontext);
    clientSocket.asyncConnect(endpoint)
            .then([&connectionEstablished]() {
                connectionEstablished = true;
            }).onError([&connectionEstablished](Error&& e) {
                connectionEstablished = false;
                FAIL() << e.toString();
            });


    iocontext.runFor(600);

    ASSERT_TRUE(connectionEstablished);
    ASSERT_TRUE(connectionAccepted);
}


TEST(TestTcpSocket, testAsyncAcceptSyncConnect) {
    EventLoop iocontext;

    TcpSocket serverSocket(iocontext);
    std::atomic_bool connectionEstablished { false };
    std::atomic_bool connectionAccepted { false };

    TcpAcceptor acceptor(iocontext, 0);
    acceptor.asyncAccept(serverSocket)
            .then([&connectionAccepted]() {
                connectionAccepted.store(true);
            }).onError([&connectionAccepted](Error&& e) {
                connectionAccepted.store(false);
                FAIL() << e.toString();
            });

    // Note: We run event loop in a separate thread because connect is blocking
    std::thread eventLoopThead([&iocontext]() {
        try {
            iocontext.runFor(600);
        } catch (const std::exception& ex) {
            FAIL() << "Running event loop failed: " << ex.what();
        }
    });

    // Give eventLoop thread chance to start
    std::this_thread::yield();

    const IPEndpoint endpoint {"127.0.0.1", acceptor.getLocalEndpoint().getPort()};
    TcpSocket clientSocket(iocontext);
    clientSocket.connect(endpoint)
            .then([&connectionEstablished]() {
                connectionEstablished.store(true);
            }).orElse([&connectionEstablished](Error&& e) {
                connectionEstablished.store(false);
                FAIL() << e.toString();
            });

    eventLoopThead.join();

    ASSERT_TRUE(connectionEstablished.load());
    ASSERT_TRUE(connectionAccepted.load());
}


TEST(TestTcpSocket, testAsyncReadWrite) {
    EventLoop iocontext;
    TcpSocket ioTcpServerSocket(iocontext);

    char message[] = "Hello there!";
    const ByteBuffer::size_type messageLen = strlen(message) + 1;
    auto messageBuffer = ByteBuffer(wrapMemory(message));

    char rcv_buffer[128];
    auto readBuffer = ByteBuffer(wrapMemory(rcv_buffer));

    bool connectionEstablished = false;
    bool connectionAccepted = false;
    bool readComplete = false;
    bool writeComplete = false;

    TcpAcceptor acceptor(iocontext, 0);
    acceptor.asyncAccept(ioTcpServerSocket)
            .then([&connectionAccepted, &ioTcpServerSocket, &readBuffer, &messageLen, &readComplete]() {
                connectionAccepted = true;

                ioTcpServerSocket.asyncRead(readBuffer, messageLen).then([&readComplete]() {
                    readComplete = true;
                });
            }).onError([&connectionAccepted](Error&& e) {
                connectionAccepted = false;
                FAIL() << e.toString();
            });


    TcpSocket clientSocket(iocontext);
    clientSocket.asyncConnect(acceptor.getLocalEndpoint())
            .then([&connectionEstablished]() {
                connectionEstablished = true;
            }).onError([&connectionEstablished](Error&& e) {
                connectionEstablished = false;
                FAIL() << e.toString();
            });

    clientSocket.asyncWrite(messageBuffer)
            .then([&writeComplete]() {
                writeComplete = true;
            }).onError([&writeComplete](Error&& e) {
                writeComplete = false;
                FAIL() << e.toString();
            });


    iocontext.runFor(600);

    ASSERT_TRUE(connectionEstablished);
    ASSERT_TRUE(connectionAccepted);

    // Check that we have read something
    ASSERT_TRUE(writeComplete);
    ASSERT_TRUE(readComplete);

    // Check that we read as much as was written
    ASSERT_FALSE(messageBuffer.hasRemaining());
    ASSERT_EQ(messageLen, messageBuffer.position());
    ASSERT_EQ(messageLen, readBuffer.position());
}
