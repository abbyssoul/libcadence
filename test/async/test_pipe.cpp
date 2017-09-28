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
 * @file: test/async/test_pipe.cpp
 * @author: soultaker
 *
 * Created on: 10/10/2016
 *******************************************************************************/
#include <cadence/async/pipe.hpp>  // Class being tested

#include <cppunit/extensions/HelperMacros.h>

using namespace Solace;
using namespace cadence::async;

class TestAsyncPipe: public CppUnit::TestFixture {

	CPPUNIT_TEST_SUITE(TestAsyncPipe);
		CPPUNIT_TEST(testAsyncWrite);
		CPPUNIT_TEST(testAsyncRead);
		CPPUNIT_TEST(testAsyncReadWrite);
	CPPUNIT_TEST_SUITE_END();

protected:
public:

	void testAsyncWrite() {
        EventLoop iocontext;
        Pipe iopipe(iocontext);

        char message[] = "Hello there!";
        const ByteBuffer::size_type messageLen = strlen(message) + 1;
        auto buffer = ByteBuffer(wrapMemory(message));

        bool writeComplete = false;

        CPPUNIT_ASSERT(!writeComplete);
        iopipe.asyncWrite(buffer).then([&writeComplete]() {
            writeComplete = true;
        });

        iocontext.runFor(300);

        CPPUNIT_ASSERT(writeComplete);
        CPPUNIT_ASSERT_EQUAL(messageLen, buffer.position());
    }


	void testAsyncRead() {
        EventLoop iocontext;
        Pipe iopipe(iocontext);

        char message[] = "Hello there!";
        const ByteBuffer::size_type messageLen = strlen(message) + 1;
        auto messageBuffer = ByteBuffer(wrapMemory(message));

        char rcv_buffer[128];
        auto readBuffer = ByteBuffer(wrapMemory(rcv_buffer));

        bool readComplete = false;
        bool writeComplete = false;

        iopipe.asyncWrite(messageBuffer).then([&writeComplete]() {
            writeComplete = true;
        });

        iopipe.asyncRead(readBuffer).then([&readComplete]() {
            readComplete = true;
        });

        iocontext.runFor(300);

        CPPUNIT_ASSERT_EQUAL(true, writeComplete);
        CPPUNIT_ASSERT_EQUAL(true, readComplete);

        // Check that we read as much as was written
        CPPUNIT_ASSERT_EQUAL(false, messageBuffer.hasRemaining());
        CPPUNIT_ASSERT_EQUAL(messageLen, messageBuffer.position());

        CPPUNIT_ASSERT_EQUAL(messageLen, readBuffer.position());
    }

	void testAsyncReadWrite() {
        EventLoop iocontext;
        Pipe iopipe(iocontext);

        char message[] = "Hello there!";
        const ByteBuffer::size_type messageLen = strlen(message) + 1;
        auto messageBuffer = ByteBuffer(wrapMemory(message));

        char rcv_buffer[128];
        auto readBuffer = ByteBuffer(wrapMemory(rcv_buffer));

        bool readComplete = false;
        bool writeComplete = false;

        iopipe.asyncRead(readBuffer).then([&readComplete, &iocontext]() {
            readComplete = true;
        });


        iopipe.asyncWrite(messageBuffer).then([&writeComplete]() {
            writeComplete = true;
        });

        iocontext.runFor(300);

        // Check that we have read something
        CPPUNIT_ASSERT_EQUAL(true, writeComplete);
        CPPUNIT_ASSERT_EQUAL(true, readComplete);

        // Check that we read as much as was written
        CPPUNIT_ASSERT_EQUAL(false, messageBuffer.hasRemaining());
        CPPUNIT_ASSERT_EQUAL(messageLen, messageBuffer.position());

        CPPUNIT_ASSERT_EQUAL(messageLen, readBuffer.position());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestAsyncPipe);
