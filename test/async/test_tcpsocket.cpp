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
#include <cadence/async/streamsocket.hpp>  // Class being tested
#include <cadence/async/acceptor.hpp>


#include <solace/output_utils.hpp>

#include "gtest/gtest.h"

#include <thread>
#include <unistd.h>
#include <fcntl.h>

using namespace Solace;
using namespace cadence;
using namespace cadence::async;

IPEndpoint anyIPEndpoint() {
    return IPEndpoint{IPAddress::any(), 0};
}

TEST(TestTcpSocket, testAsyncConnect) {
    EventLoop iocontext;

    bool connectionEstablished = false;
    bool connectionAccepted = false;

    Acceptor acceptor(iocontext);
    ASSERT_TRUE(acceptor.open(anyIPEndpoint()).isOk());

    acceptor.asyncAccept()
            .then([&connectionAccepted](StreamSocket&& ) {
                connectionAccepted = true;
            }).onError([&connectionAccepted](Error&& e) {
                connectionAccepted = false;
                FAIL() << e.toString();
            });

    auto clientSocket = createTCPSocket(iocontext);
    clientSocket.asyncConnect(acceptor.getLocalEndpoint())
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

    std::atomic_bool connectionEstablished { false };
    std::atomic_bool connectionAccepted { false };

    Acceptor acceptor(iocontext);
    ASSERT_TRUE(acceptor.open(anyIPEndpoint()).isOk());

    acceptor.asyncAccept()
            .then([&connectionAccepted](StreamSocket&& ) {
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

    auto clientSocket = createTCPSocket(iocontext);
    clientSocket.connect(acceptor.getLocalEndpoint())
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

    char message[] = "Hello there!";
    auto const messageLen = strlen(message) + 1;
    auto messageBuffer = ByteReader(wrapMemory(message));

    char rcv_buffer[128];
    auto readBuffer = ByteWriter(wrapMemory(rcv_buffer));

    bool connectionEstablished = false;
    bool connectionAccepted = false;
    bool readComplete = false;
    bool writeComplete = false;

    Acceptor acceptor(iocontext);
    ASSERT_TRUE(acceptor.open(anyIPEndpoint()).isOk());

    acceptor.asyncAccept()
            .then([&connectionAccepted, &readBuffer, &messageLen, &readComplete](StreamSocket&& sock) {
                connectionAccepted = true;

                sock.asyncRead(readBuffer, messageLen)
                        .then([&readComplete, c = std::move(sock)]() {
                            readComplete = true;
                        });
            }).onError([&connectionAccepted](Error&& e) {
                connectionAccepted = false;
                FAIL() << e.toString();
            });


    auto clientSocket = createTCPSocket(iocontext);
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
