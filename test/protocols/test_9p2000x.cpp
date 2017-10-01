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


inline uint16 operator "" _us(unsigned long long value) {  // NOLINT(runtime/int)
    return static_cast<uint16>(value);
}


namespace cadence {
    std::ostream& operator<< (std::ostream& ostr, P9Protocol::MessageType t) {

        switch (t) {
        case P9Protocol::MessageType::TVersion: ostr << "TVersion"; break;
        case P9Protocol::MessageType::RVersion: ostr << "RVersion"; break;
        case P9Protocol::MessageType::TAuth:    ostr << "TAuth"; break;
        case P9Protocol::MessageType::RAuth:    ostr << "RAuth"; break;
        case P9Protocol::MessageType::TAttach:  ostr << "TAttach"; break;
        case P9Protocol::MessageType::RAttach:  ostr << "RAttach"; break;
        case P9Protocol::MessageType::TError:   ostr << "TError"; break;
        case P9Protocol::MessageType::RError:   ostr << "RError"; break;
        case P9Protocol::MessageType::TFlush:   ostr << "TFlush"; break;
        case P9Protocol::MessageType::RFlush:   ostr << "RFlush"; break;
        case P9Protocol::MessageType::TWalk:    ostr << "TWalk"; break;
        case P9Protocol::MessageType::RWalk:    ostr << "RWalk"; break;
        case P9Protocol::MessageType::TOpen:    ostr << "TOpen"; break;
        case P9Protocol::MessageType::ROpen:    ostr << "ROpen"; break;
        case P9Protocol::MessageType::TCreate:  ostr << "TCreate"; break;
        case P9Protocol::MessageType::RCreate:  ostr << "RCreate"; break;
        case P9Protocol::MessageType::TRead:    ostr << "TRead"; break;
        case P9Protocol::MessageType::RRead:    ostr << "RRead"; break;
        case P9Protocol::MessageType::TWrite:   ostr << "TWrite"; break;
        case P9Protocol::MessageType::RWrite:   ostr << "RWrite"; break;
        case P9Protocol::MessageType::TClunk:   ostr << "TClunk"; break;
        case P9Protocol::MessageType::RClunk:   ostr << "RClunk"; break;
        case P9Protocol::MessageType::TRemove:  ostr << "TRemove"; break;
        case P9Protocol::MessageType::RRemove:  ostr << "RRemove"; break;
        case P9Protocol::MessageType::TStat:    ostr << "TStat"; break;
        case P9Protocol::MessageType::RStat:    ostr << "RStat"; break;
        case P9Protocol::MessageType::TWStat:   ostr << "TWStat"; break;
        case P9Protocol::MessageType::RWStat:   ostr << "RWStat"; break;
        default:
            ostr << "[Unknown value '" << static_cast<byte>(t) << "']";
        }

        return ostr;
    }
}  // end of namespace cadence


TEST(P9p2000x, testHeaderSize) {
    P9Protocol proc;

    ASSERT_EQ(4u + 1u + 2u, proc.headerSize());
}

TEST(P9p2000x, testSettingFrameSize) {
    P9Protocol proc(127);

    ASSERT_EQ(127u, proc.maxPossibleMessageSize());
    ASSERT_EQ(127u, proc.maxNegotiatedMessageSize());

    proc.maxNegotiatedMessageSize(56);
    ASSERT_EQ(127u, proc.maxPossibleMessageSize());
    ASSERT_EQ(56u, proc.maxNegotiatedMessageSize());

    ASSERT_ANY_THROW(proc.maxNegotiatedMessageSize(300));
}

TEST(P9p2000x, testParsingMessageHeader) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    // Form a normal message with no data:
    ByteBuffer buffer(_mem.create(512));
    buffer << P9Protocol::size_type(4 + 1 + 2);
    buffer << static_cast<byte>(P9Protocol::MessageType::TVersion);
    buffer << P9Protocol::Tag(1);

    auto res = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(res.isOk());

    auto header = res.unwrap();
    ASSERT_EQ(4u + 1u + 2u, header.size);
    ASSERT_EQ(P9Protocol::MessageType::TVersion, header.type);
    ASSERT_EQ(1_us, header.tag);
}

TEST(P9p2000x, testParsingHeaderWithInsufficientData) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    ByteBuffer buffer(_mem.create(512));
    // Only write one header field. Should be not enough data to read a header.
    buffer << P9Protocol::size_type(4 + 1 + 2);
    auto res = proc.parseMessageHeader(buffer.flip());

    ASSERT_TRUE(res.isError());
}


TEST(P9p2000x, testParsingIllformedMessageHeader) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    ByteBuffer buffer(_mem.create(512));
    // Set declared message size less then header size.
    buffer << P9Protocol::size_type(1 + 2);
    buffer << static_cast<byte>(P9Protocol::MessageType::TVersion);
    buffer << P9Protocol::Tag(1);

    auto res = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(res.isError());
}

TEST(P9p2000x, testParsingIllformedHeaderForMessagesLargerMTU) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    ByteBuffer buffer(_mem.create(512));
    proc.maxNegotiatedMessageSize(20);
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(proc.maxNegotiatedMessageSize() + 100);
    buffer << static_cast<byte>(P9Protocol::MessageType::TVersion);
    buffer << P9Protocol::Tag(1);

    auto res = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(res.isError());
}


TEST(P9p2000x, createVersionRequest) {
    MemoryManager _mem(1024);
    ByteBuffer buffer(_mem.create(512));

    const String testVersion = P9Protocol::PROTOCOL_VERSION;
    const uint64 versionStringLen = testVersion.size();

    P9Protocol proc;
    proc.createVersionRequest(0, buffer, testVersion);

    ASSERT_EQ(proc.headerSize() + 4 + 2 + versionStringLen, buffer.position());
}

