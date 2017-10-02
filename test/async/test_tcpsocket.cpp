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

#include <unistd.h>
#include <fcntl.h>

using namespace Solace;
using namespace cadence;
using namespace cadence::async;

TEST(TestTcpSocket, testAsyncReadWrite) {
    EventLoop iocontext;
    bool connectionAccepted = false;

    TcpSocket ioTcpServerSocket(iocontext);
    TcpSocket ioTcpClientSocket(iocontext);
    TcpAcceptor acceptor(iocontext, 20000);

    IPEndpoint endpoint("127.0.0.1", 20000);
    ioTcpClientSocket.connect(endpoint);

    char message[] = "Hello there!";
    const ByteBuffer::size_type messageLen = strlen(message) + 1;
    auto messageBuffer = ByteBuffer(wrapMemory(message));

    char rcv_buffer[128];
    auto readBuffer = ByteBuffer(wrapMemory(rcv_buffer));

    bool readComplete = false;
    bool writeComplete = false;

    acceptor.asyncAccept(ioTcpServerSocket)
            .then([&connectionAccepted, &ioTcpServerSocket, &readBuffer, &messageLen, &readComplete]() {
        connectionAccepted = true;

        ioTcpServerSocket.asyncRead(readBuffer, messageLen).then([&readComplete]() {
            readComplete = true;
        });
    });

    ioTcpClientSocket.asyncWrite(messageBuffer).then([&writeComplete]() {
        writeComplete = true;
    });

    iocontext.runFor(300);

    ASSERT_TRUE(connectionAccepted);

    // Check that we have read something
    ASSERT_TRUE(writeComplete);
    ASSERT_TRUE(readComplete);

    // Check that we read as much as was written
    ASSERT_FALSE(messageBuffer.hasRemaining());
    ASSERT_EQ(messageLen, messageBuffer.position());
    ASSERT_EQ(messageLen, readBuffer.position());
}
