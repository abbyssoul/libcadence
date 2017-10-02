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

void writeStat(ByteBuffer& dest, const P9Protocol::Stat& stat);

namespace cadence {

    bool operator == (const P9Protocol::Stat& lhs, const P9Protocol::Stat& rhs) {
        return (lhs.atime == rhs.atime &&
                lhs.dev == rhs.dev &&
                lhs.gid == rhs.gid &&
                lhs.length == rhs.length &&
                lhs.mode == rhs.mode &&
                lhs.mtime == rhs.mtime &&
                lhs.name == rhs.name &&
                lhs.qid.path == rhs.qid.path &&
                lhs.qid.version == rhs.qid.version &&
                lhs.qid.type == rhs.qid.type &&
                lhs.size == rhs.size &&
                lhs.type == rhs.type &&
                lhs.uid == rhs.uid);
    }

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


TEST(P9p2000x, parsingMessageHeaderWithUnknownMessageType) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    // Form a normal message with no data:
    ByteBuffer buffer(_mem.create(512));
    buffer << P9Protocol::size_type(4 + 1 + 2);
    buffer << static_cast<byte>(-1);
    buffer << P9Protocol::Tag(1);

    ASSERT_TRUE(proc.parseMessageHeader(buffer.flip()).isError());
//    ASSERT_ANY_THROW(proc.parseMessageHeader(buffer.flip()));
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

    ByteBuffer buffer(_mem.create(512));
    // Set declared message size less then header size.
    buffer << P9Protocol::size_type(1 + 2);
    buffer << static_cast<byte>(P9Protocol::MessageType::TVersion);
    buffer << P9Protocol::Tag(1);

    P9Protocol proc;
    ASSERT_TRUE(proc.parseMessageHeader(buffer.flip()).isError());
}

TEST(P9p2000x, parsingIllFormedHeaderForMessagesLargerMTUShouldError) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    ByteBuffer buffer(_mem.create(512));
    proc.maxNegotiatedMessageSize(20);
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(proc.maxNegotiatedMessageSize() + 100);
    buffer << static_cast<byte>(P9Protocol::MessageType::TVersion);
    buffer << P9Protocol::Tag(1);

    ASSERT_TRUE(proc.parseMessageHeader(buffer.flip()).isError());
}


TEST(P9p2000x, parseIncorrectlySizedSmallerResponse) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    ByteBuffer buffer(_mem.create(512));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(proc.headerSize() + sizeof(int32));
    buffer << static_cast<byte>(P9Protocol::MessageType::RVersion);
    buffer << P9Protocol::Tag(1);
    buffer << byte(3);

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isError());
}

TEST(P9p2000x, parseIncorrectlySizedLargerResponse) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    ByteBuffer buffer(_mem.create(proc.headerSize() + sizeof(int32)*2));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(proc.headerSize() + sizeof(int32));
    buffer << static_cast<byte>(P9Protocol::MessageType::RVersion);
    buffer << P9Protocol::Tag(1);
    buffer << int64(999999);

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isError());
}

TEST(P9p2000x, parseVersionRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + sizeof(int32) + sizeof(int16) + 2;
    ByteBuffer buffer(_mem.create(messageSize));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(messageSize);
    buffer << static_cast<byte>(P9Protocol::MessageType::RVersion);
    buffer << P9Protocol::Tag(1);
    buffer << int32(512);
    buffer << int16(2);
    buffer.write("9P", 2);

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();

    ASSERT_EQ(512, response.version.msize);
    ASSERT_STREQ("9P", response.version.version.c_str());
}

TEST(P9p2000x, parseAuthRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + 13;
    ByteBuffer buffer(_mem.create(messageSize));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(messageSize);
    buffer << static_cast<byte>(P9Protocol::MessageType::RAuth);
    buffer << P9Protocol::Tag(1);

    buffer << byte(13);     // QID.type
    buffer << uint32(91);   // QID.version
    buffer << uint64(441);  // QID.path

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(13, response.auth.qid.type);
    EXPECT_EQ(91, response.auth.qid.version);
    EXPECT_EQ(441, response.auth.qid.path);
}

TEST(P9p2000x, parseErrorRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    const char* expectedErrorMessage = "All good!";
    const uint16 messageLen = static_cast<uint16>(strlen(expectedErrorMessage));
    const auto messageSize = proc.headerSize() + sizeof(uint16) + messageLen;
    ByteBuffer buffer(_mem.create(messageSize));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(messageSize);
    buffer << static_cast<byte>(P9Protocol::MessageType::RError);
    buffer << P9Protocol::Tag(1);

    buffer << messageLen;
    buffer.write(expectedErrorMessage, messageLen);

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isError());

    auto response = message.moveError();
    EXPECT_STREQ(expectedErrorMessage, response.toString().c_str());
//    EXPECT_STREQ(expectedErrorMessage, response.error.ename.c_str());
}


TEST(P9p2000x, parseFlushRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    const auto messageSize = proc.headerSize();
    ByteBuffer buffer(_mem.create(messageSize));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(messageSize);
    buffer << static_cast<byte>(P9Protocol::MessageType::RFlush);
    buffer << P9Protocol::Tag(1);

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isOk());
}

TEST(P9p2000x, parseAttachRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + 13;
    ByteBuffer buffer(_mem.create(messageSize));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(messageSize);
    buffer << static_cast<byte>(P9Protocol::MessageType::RAttach);
    buffer << P9Protocol::Tag(1);

    buffer << byte(81);     // QID.type
    buffer << uint32(3);   // QID.version
    buffer << uint64(1049);  // QID.path

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(81, response.attach.qid.type);
    EXPECT_EQ(3, response.attach.qid.version);
    EXPECT_EQ(1049, response.attach.qid.path);
}

TEST(P9p2000x, parseOpenRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + 13 + sizeof(uint32);
    ByteBuffer buffer(_mem.create(messageSize));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(messageSize);
    buffer << static_cast<byte>(P9Protocol::MessageType::ROpen);
    buffer << P9Protocol::Tag(1);
    // qid
    buffer << byte(71);     // QID.type
    buffer << uint32(33);   // QID.version
    buffer << uint64(4173);  // QID.path
    // iounit
    buffer << uint32(998);

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(71, response.open.qid.type);
    EXPECT_EQ(33, response.open.qid.version);
    EXPECT_EQ(4173, response.open.qid.path);
    EXPECT_EQ(998, response.open.iounit);
}

TEST(P9p2000x, parseCreateRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + 13 + sizeof(uint32);
    ByteBuffer buffer(_mem.create(messageSize));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(messageSize);
    buffer << static_cast<byte>(P9Protocol::MessageType::RCreate);
    buffer << P9Protocol::Tag(1);
    // qid
    buffer << byte(87);     // QID.type
    buffer << uint32(5481);   // QID.version
    buffer << uint64(17);  // QID.path
    // iounit
    buffer << uint32(778);

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(87, response.create.qid.type);
    EXPECT_EQ(5481, response.create.qid.version);
    EXPECT_EQ(17, response.create.qid.path);
    EXPECT_EQ(778, response.create.iounit);
}

TEST(P9p2000x, parseReadRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    const char* messageData = "This is a very important data d-_^b";
    const uint32 dataLen = static_cast<uint32>(strlen(messageData));
    const auto messageSize = proc.headerSize() + sizeof(uint32) + dataLen;
    ByteBuffer buffer(_mem.create(messageSize));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(messageSize);
    buffer << static_cast<byte>(P9Protocol::MessageType::RRead);
    buffer << P9Protocol::Tag(1);
    // iounit
    buffer << dataLen;
    buffer.write(messageData, dataLen);

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(dataLen, response.read.data.size());
    EXPECT_EQ(0, memcmp(response.read.data.dataAddress(), messageData, dataLen));
}


TEST(P9p2000x, parseWriteRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + sizeof(uint32);
    ByteBuffer buffer(_mem.create(messageSize));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(messageSize);
    buffer << static_cast<byte>(P9Protocol::MessageType::RWrite);
    buffer << P9Protocol::Tag(1);
    // iounit
    buffer << uint32(81177);

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(81177, response.write.count);
}


TEST(P9p2000x, parseClunkRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    const auto messageSize = proc.headerSize();
    ByteBuffer buffer(_mem.create(messageSize));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(messageSize);
    buffer << static_cast<byte>(P9Protocol::MessageType::RClunk);
    buffer << P9Protocol::Tag(1);

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isOk());
}

TEST(P9p2000x, parseRemoveRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    const auto messageSize = proc.headerSize();
    ByteBuffer buffer(_mem.create(messageSize));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(messageSize);
    buffer << static_cast<byte>(P9Protocol::MessageType::RRemove);
    buffer << P9Protocol::Tag(1);

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isOk());
}

TEST(P9p2000x, parseStatRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + 512;
    ByteBuffer buffer(_mem.create(messageSize));
    // Set declared message size to be more then negotiated message size
    const auto headPosition = buffer.position();
    buffer << P9Protocol::size_type(0);
    buffer << static_cast<byte>(P9Protocol::MessageType::RStat);
    buffer << P9Protocol::Tag(1);

    P9Protocol::Stat stat;
    stat.atime = 21;
    stat.dev = 8828;
    stat.gid = "Some user";
    stat.length = 818177;
    stat.mode = 111;
    stat.mtime = 17;
    stat.name = "File McFileface";
    stat.qid.path = 61;
    stat.qid.type = 15;
    stat.qid.version = 404;
    stat.size = 124;
    stat.type = 1;
    stat.uid = "User McUserface";

    writeStat(buffer, stat);
    const auto totalSize = buffer.position();
    buffer.reset(headPosition) << P9Protocol::size_type(totalSize);
    buffer.reset(totalSize);

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(stat, response.stat);
}

TEST(P9p2000x, parseWStatRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    const auto messageSize = proc.headerSize();
    ByteBuffer buffer(_mem.create(messageSize));
    // Set declared message size to be more then negotiated message size
    buffer << P9Protocol::size_type(messageSize);
    buffer << static_cast<byte>(P9Protocol::MessageType::RWStat);
    buffer << P9Protocol::Tag(1);

    auto header = proc.parseMessageHeader(buffer.flip());
    ASSERT_TRUE(header.isOk());

    auto message = proc.parseMessage(header.unwrap(), buffer);
    ASSERT_TRUE(message.isOk());
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
