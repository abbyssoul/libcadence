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
 * @file: test/async/test_udpsocket.cpp
 * @author: soultaker
 *
 * Created on: 10/10/2016
 *******************************************************************************/
#include <cadence/async/udpsocket.hpp>  // Class being tested

#include "gtest/gtest.h"

#include <unistd.h>

using namespace Solace;
using namespace cadence::async;


TEST(TestUdpSocket, testAsyncReadWrite) {
    EventLoop iocontext;
    UdpSocket udpServerSocket(iocontext, 20000);
    UdpSocket udpClientSocket(iocontext, 20001);

    char message[] = "Hello there!";
    const ByteBuffer::size_type messageLen = strlen(message) + 1;

    auto messageBuffer = ByteBuffer(wrapMemory(message));

    char rcv_buffer[128];
    auto readBuffer = ByteBuffer(wrapMemory(rcv_buffer));

    bool readComplete = false;
    bool writeComplete = false;

    UdpSocketReceiver receiver("localhost", "20001");
    udpServerSocket.asyncWrite(messageBuffer, receiver).then([&writeComplete]() {
        writeComplete = true;
    });

    // udpServerSocket.asyncWrite(messageBuffer, "localhost", "20001").then([&writeComplete]() {
    // 	writeComplete = true;
    // });

    udpClientSocket.asyncRead(readBuffer, messageLen).then([&readComplete]() {
        readComplete = true;
    });

    iocontext.runFor(300);

    // Check that we have read something
    ASSERT_TRUE(writeComplete);
    ASSERT_TRUE(readComplete);

    // Check that we read as much as was written
    ASSERT_FALSE(messageBuffer.hasRemaining());
    ASSERT_EQ(messageLen, messageBuffer.position());
    ASSERT_EQ(messageLen, readBuffer.position());
}
