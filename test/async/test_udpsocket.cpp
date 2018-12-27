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

#include <solace/output_utils.hpp>

#include "gtest/gtest.h"


using namespace Solace;
using namespace cadence;
using namespace cadence::async;


TEST(TestUdpSocket, testAsyncReadWrite) {
    EventLoop iocontext;

    char const message[] = "Hello there!";
    auto messageBuffer = ByteReader(wrapMemory(message));
    auto const messageLen = messageBuffer.limit();

    char rcv_buffer[128];
    auto readBuffer = ByteWriter(wrapMemory(rcv_buffer));

    bool readComplete = false;
    bool writeComplete = false;

    auto udpServer = UdpSocket(iocontext, 20000);
    ASSERT_TRUE(udpServer.isOpen());
    udpServer.asyncReadFrom(readBuffer)
            .then([&readComplete](IPEndpoint&&) {
                readComplete = true;
            })
            .onError([](Error&& e) {
                FAIL() << e.toString();
            });


    auto udpClient = UdpSocket{iocontext};
    ASSERT_TRUE(udpClient.isOpen());
    auto const destAddr = udpServer.getLocalEndpoint();
    udpClient.asyncWriteTo(destAddr, messageBuffer)
            .then([&writeComplete]() {
                writeComplete = true;
            })
            .onError([](Error&& e) {
                FAIL() << e.toString();
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
