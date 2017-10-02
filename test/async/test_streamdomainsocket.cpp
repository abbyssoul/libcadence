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
 * @file: test/async/test_streamdomainsocket.cpp
 * @author: soultaker
 *
 * Created on: 10/10/2016
 *******************************************************************************/
#include <cadence/async/streamdomainsocket.hpp>  // Class being tested

#include "gtest/gtest.h"


using namespace Solace;
using namespace cadence::async;


class TestStreamDomainSocket : public ::testing::Test {
protected:

    void SetUp() override {
        unlink(testSocketName);
    }

    void TearDown() override {
        unlink(testSocketName);
    }

    const char* testSocketName = "/tmp/cadence.test.StreamDomainSocket";
    EventLoop iocontext;
};



TEST_F(TestStreamDomainSocket, testAsyncReadWrite) {
    String endpoint(testSocketName);

    StreamDomainAcceptor acceptor(iocontext, endpoint);
    StreamDomainSocket ioStreamServerSocket(iocontext);
    StreamDomainSocket ioStreamClientSocket(iocontext);

    bool connectionAccepted = false;
    char message[] = "Hello there!";
    const ByteBuffer::size_type messageLen = strlen(message) + 1;
    auto messageBuffer = ByteBuffer(wrapMemory(message));

    char rcv_buffer[128];
    auto readBuffer = ByteBuffer(wrapMemory(rcv_buffer));

    bool readComplete = false;
    bool writeComplete = false;

    acceptor.asyncAccept(ioStreamServerSocket)
            .then([&connectionAccepted, &ioStreamServerSocket, &readBuffer, &messageLen, &readComplete]() {
        connectionAccepted = true;

        ioStreamServerSocket.asyncRead(readBuffer, messageLen).then([&readComplete]() {
            readComplete = true;
        });
    });

    ioStreamClientSocket.connect(endpoint);
    ioStreamClientSocket.asyncWrite(messageBuffer).then([&writeComplete]() {
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
