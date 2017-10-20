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
 * @file: test/async/test_datagramdomainsocketsocket.cpp
 * @author: soultaker
 *
 * Created on: 10/10/2016
 *******************************************************************************/
#include <cadence/async/datagramdomainsocket.hpp>  // Class being tested

#include "gtest/gtest.h"

using namespace Solace;
using namespace cadence;
using namespace cadence::async;


class TestDatagramDomainSocket : public ::testing::Test {
protected:

    void SetUp() override {
        unlink(testClientSocketNameStr);
        unlink(testServerSocketNameStr);
    }

    void TearDown() override {
        unlink(testClientSocketNameStr);
        unlink(testServerSocketNameStr);
    }

    const char* testClientSocketNameStr = "/tmp/cadence.test.client";
    const char* testServerSocketNameStr = "/tmp/cadence.test.server";
    EventLoop iocontext;
};


TEST_F(TestDatagramDomainSocket, asyncReadWrite) {
    UnixEndpoint testClientSocketName(testClientSocketNameStr);
    UnixEndpoint testServerSocketName(testServerSocketNameStr);

    DatagramDomainSocket serverSocket(iocontext, testServerSocketName);
    DatagramDomainSocket clientSocket(iocontext, testClientSocketName);

    char message[] = "Hello there!";
    const ByteBuffer::size_type messageLen = strlen(message) + 1;

    auto messageBuffer = ByteBuffer(wrapMemory(message));

    char rcv_buffer[128];
    auto readBuffer = ByteBuffer(wrapMemory(rcv_buffer));

    bool readComplete = false;
    bool writeComplete = false;

    clientSocket.asyncWriteTo(messageBuffer, messageLen, testServerSocketName)
        .then([&writeComplete]() {
            writeComplete = true;
        }).onError([](Error&& e) {
            ADD_FAILURE() << e.toString().to_str();
        });

    serverSocket.asyncRead(readBuffer, messageLen)
        .then([&readComplete]() {
            readComplete = true;
        }).onError([](Error&& e) {
            ADD_FAILURE() << e.toString().to_str();
        });

    iocontext.runFor(300);

    ASSERT_TRUE(writeComplete);
    ASSERT_TRUE(readComplete);
}
