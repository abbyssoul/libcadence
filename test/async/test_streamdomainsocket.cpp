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
#include <cadence/async/streamsocket.hpp>  // Class being tested
#include <cadence/async/acceptor.hpp>

#include <solace/output_utils.hpp>

#include "gtest/gtest.h"


using namespace Solace;
using namespace cadence;
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
    Acceptor acceptor(iocontext);
    auto ioStreamClientSocket = createUnixSocket(iocontext);

    bool connectionAccepted = false;
    char message[] = "Hello there!";
    auto const messageLen = strlen(message) + 1;
    auto messageBuffer = ByteReader(wrapMemory(message));

    char rcv_buffer[128];
    auto readBuffer = ByteWriter(wrapMemory(rcv_buffer));

    bool readComplete = false;
    bool writeComplete = false;

    NetworkEndpoint endpoint(UnixEndpoint(makeString(testSocketName)));
    ASSERT_TRUE(acceptor.open(endpoint));

    acceptor.asyncAccept()
            .then([&](StreamSocket&& peer) {
                connectionAccepted = true;

                peer.asyncRead(readBuffer, messageLen).then([&readComplete]() {
                    readComplete = true;
                });
            });

    ASSERT_TRUE(ioStreamClientSocket.connect(endpoint));

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
