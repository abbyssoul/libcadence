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

#include "gtest/gtest.h"


using namespace Solace;
using namespace cadence::async;

TEST(TestAsyncPipe, testAsyncWrite) {
    EventLoop iocontext;
    Pipe iopipe(iocontext);

    char message[] = "Hello there!";
    const ByteBuffer::size_type messageLen = strlen(message) + 1;
    auto buffer = ByteBuffer(wrapMemory(message));

    bool writeComplete = false;

    iopipe.asyncWrite(buffer).then([&writeComplete]() {
        writeComplete = true;
    });

    iocontext.runFor(300);

    ASSERT_TRUE(writeComplete);
    ASSERT_EQ(messageLen, buffer.position());
}


TEST(TestAsyncPipe, testAsyncRead) {
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

    ASSERT_TRUE(writeComplete);
    ASSERT_TRUE(readComplete);

    // Check that we read as much as was written
    ASSERT_FALSE(messageBuffer.hasRemaining());
    ASSERT_EQ(messageLen, messageBuffer.position());

    ASSERT_EQ(messageLen, readBuffer.position());
}

TEST(TestAsyncPipe, testAsyncReadWrite) {
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
    ASSERT_TRUE(writeComplete);
    ASSERT_TRUE(readComplete);

    // Check that we read as much as was written
    ASSERT_FALSE(messageBuffer.hasRemaining());
    ASSERT_EQ(messageLen, messageBuffer.position());

    ASSERT_EQ(messageLen, readBuffer.position());
}
