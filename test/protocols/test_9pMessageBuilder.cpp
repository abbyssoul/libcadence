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
 * @file: test/protocols/test_9p2000x.cpp
 * @author: soultaker
 *
 *******************************************************************************/
#include <cadence/protocols/9p2000x.hpp>  // Class being tested

#include <solace/exception.hpp>

#include "gtest/gtest.h"


using namespace Solace;
using namespace cadence;


class P9MessageBuilder : public ::testing::Test {
public:
    P9MessageBuilder() :
        _memManager(P9Protocol::MAX_MESSAGE_SIZE)
    {}


protected:

    void SetUp() override {
        _buffer = _memManager.create(P9Protocol::MAX_MESSAGE_SIZE);
        _buffer.viewRemaining().fill(0xFE);
    }

    void TearDown() override {
    }

    MemoryManager   _memManager;
    ByteBuffer      _buffer;
};



TEST_F(P9MessageBuilder, payloadResizing) {
    ImmutableMemoryView emptyBuffer;
    P9Protocol::ResponseBuilder builder(_buffer);

    builder.read(emptyBuffer);
    ASSERT_EQ(4, builder.payloadSize());
    ASSERT_EQ(P9Protocol::headerSize() + 4 + 0, _buffer.position());

    // Write extra data:
    const byte extraData[] = {1, 3, 2, 45, 18};
    builder.buffer().write(extraData, 5);

    builder.updatePayloadSize();
    ASSERT_EQ(5 + 4, builder.payloadSize());
    ASSERT_EQ(P9Protocol::headerSize() + 4 + 5, _buffer.position());

    builder.updatePayloadSize(3);
    ASSERT_EQ(3, builder.payloadSize());
    ASSERT_EQ(P9Protocol::headerSize() + 3, _buffer.position());
}



TEST_F(P9MessageBuilder, messageChanging) {
    ImmutableMemoryView emptyBuffer;
    P9Protocol::ResponseBuilder builder(_buffer);

    builder.read(emptyBuffer);
    ASSERT_EQ(P9Protocol::headerSize() + 4 + 0, _buffer.position());
    ASSERT_EQ(P9Protocol::MessageType::RRead, builder.type());

    // Change message type
    const char* message = "Nothing to read";
    const auto payloadSize = strlen(message) + 2;
    builder.error(message);

    ASSERT_EQ(payloadSize, builder.payloadSize());
    ASSERT_EQ(P9Protocol::headerSize() + payloadSize, _buffer.position());
    ASSERT_EQ(P9Protocol::MessageType::RError, builder.type());
}
