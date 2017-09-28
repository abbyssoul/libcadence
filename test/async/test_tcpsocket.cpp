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
#include <cppunit/extensions/HelperMacros.h>

#include <unistd.h>
#include <fcntl.h>

using namespace Solace;
using namespace cadence::async;

class TestTcpSocket: public CppUnit::TestFixture {

	CPPUNIT_TEST_SUITE(TestTcpSocket);
        CPPUNIT_TEST(testAsyncReadWrite);
	CPPUNIT_TEST_SUITE_END();

protected:
public:

	void testAsyncReadWrite() {
        EventLoop iocontext;
        bool connectionAccepted = false;

        TcpSocket ioTcpServerSocket(iocontext, 20000);
        TcpSocket ioTcpClientSocket(iocontext, 20000);
        TcpAcceptor acceptor(iocontext, 20000);

        String endpoint("127.0.0.1");
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

        CPPUNIT_ASSERT(connectionAccepted);

        // Check that we have read something
        CPPUNIT_ASSERT_EQUAL(true, writeComplete);
        CPPUNIT_ASSERT_EQUAL(true, readComplete);

        // Check that we read as much as was written
        CPPUNIT_ASSERT_EQUAL(false, messageBuffer.hasRemaining());
        CPPUNIT_ASSERT_EQUAL(messageLen, messageBuffer.position());
        CPPUNIT_ASSERT_EQUAL(messageLen, readBuffer.position());
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestTcpSocket);
