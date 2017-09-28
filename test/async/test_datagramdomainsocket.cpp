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
#include <cppunit/extensions/HelperMacros.h>

using namespace Solace;
using namespace cadence::async;


class TestDatagramDomainSocket : public CppUnit::TestFixture {

	CPPUNIT_TEST_SUITE(TestDatagramDomainSocket);
        CPPUNIT_TEST(testAsyncReadWrite);
	CPPUNIT_TEST_SUITE_END();

protected:
public:

    void testAsyncReadWrite() {
        EventLoop iocontext;

        const char* testClientSocketNameStr = "/tmp/socket.unix.client";
        const char* testServerSocketNameStr = "/tmp/socket.unix.server";
        unlink(testClientSocketNameStr);
        unlink(testServerSocketNameStr);

        DatagramDomainSocket::endpoint_type testClientSocketName(testClientSocketNameStr);
        DatagramDomainSocket::endpoint_type testServerSocketName(testServerSocketNameStr);

        DatagramDomainSocket udpServerSocket(iocontext, testServerSocketName);
        DatagramDomainSocket udpClientSocket(iocontext, testClientSocketName);

        char message[] = "Hello there!";
        const ByteBuffer::size_type messageLen = strlen(message) + 1;

        auto messageBuffer = ByteBuffer(wrapMemory(message));

        char rcv_buffer[128];
        auto readBuffer = ByteBuffer(wrapMemory(rcv_buffer));

        bool readComplete = false;
        bool writeComplete = false;

        udpServerSocket.asyncWrite(messageBuffer, messageLen, testClientSocketName)
            .then([&writeComplete]() {
                writeComplete = true;
            }).onError([](Error&& e) {
                CPPUNIT_FAIL(e.toString().to_str());
            });

        udpClientSocket.asyncRead(readBuffer, messageLen, testClientSocketName)
            .then([&readComplete]() {
                readComplete = true;
            }).onError([](Error&& e) {
                CPPUNIT_FAIL(e.toString().to_str());
            });

        iocontext.runFor(300);

        CPPUNIT_ASSERT_EQUAL(true, writeComplete);
        CPPUNIT_ASSERT_EQUAL(true, readComplete);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestDatagramDomainSocket);
