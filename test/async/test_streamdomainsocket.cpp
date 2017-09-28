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
#include <cppunit/extensions/HelperMacros.h>

#include <unistd.h>
#include <fcntl.h>

using namespace Solace;
using namespace cadence::async;


class TestStreamDomainSocket: public CppUnit::TestFixture {

	CPPUNIT_TEST_SUITE(TestStreamDomainSocket);
        CPPUNIT_TEST(testAsyncReadWrite);
	CPPUNIT_TEST_SUITE_END();

protected:
	public:

	void testAsyncReadWrite() {
        EventLoop iocontext;
        const char* testSocketName = "TestStreamDomainSocket";
        unlink(testSocketName);
        String endpoint(testSocketName);

        StreamDomainAcceptor acceptor(iocontext, endpoint);


        StreamDomainSocket ioStreamServerSocket(iocontext);
        StreamDomainSocket ioStreamClientSocket(iocontext);


        ioStreamClientSocket.connect(endpoint);

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

        ioStreamClientSocket.asyncWrite(messageBuffer).then([&writeComplete]() {
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

CPPUNIT_TEST_SUITE_REGISTRATION(TestStreamDomainSocket);
