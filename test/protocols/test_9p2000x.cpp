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

void encode9P(ByteBuffer& dest, const P9Protocol::Stat& stat) {
    dest << stat.size;
    dest << stat.type;
    dest << stat.dev;

    dest << stat.qid.type;
    dest << stat.qid.version;
    dest << stat.qid.path;

    dest << stat.mode;
    dest << stat.atime;
    dest << stat.mtime;
    dest << stat.length;

    dest << static_cast<uint16>(stat.name.size());
    dest.write(stat.name.c_str(), stat.name.size());

    dest << static_cast<uint16>(stat.uid.size());
    dest.write(stat.uid.c_str(), stat.uid.size());

    dest << static_cast<uint16>(stat.gid.size());
    dest.write(stat.gid.c_str(), stat.gid.size());

    dest << static_cast<uint16>(stat.muid.size());
    dest.write(stat.muid.c_str(), stat.muid.size());
}

namespace cadence {

bool operator == (const P9Protocol::Qid& lhs, const P9Protocol::Qid& rhs) {
    return (lhs.path == rhs.path &&
            lhs.version == rhs.version &&
            lhs.type == rhs.type);
}

bool operator == (const P9Protocol::Stat& lhs, const P9Protocol::Stat& rhs) {
    return (lhs.atime == rhs.atime &&
            lhs.dev == rhs.dev &&
            lhs.gid == rhs.gid &&
            lhs.length == rhs.length &&
            lhs.mode == rhs.mode &&
            lhs.mtime == rhs.mtime &&
            lhs.name == rhs.name &&
            lhs.qid == rhs.qid &&
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

        case P9Protocol::MessageType::TSession: ostr << "TSession"; break;
        case P9Protocol::MessageType::RSession: ostr << "RSession"; break;
        case P9Protocol::MessageType::TSRead:   ostr << "TSRead"; break;
        case P9Protocol::MessageType::RSRead:   ostr << "RSRead"; break;
        case P9Protocol::MessageType::TSWrite:  ostr << "TSWrite"; break;
        case P9Protocol::MessageType::RSWrite:  ostr << "RSWrite"; break;
        default:
            ostr << "[Unknown value '" << static_cast<byte>(t) << "']";
        }

        return ostr;
    }
}  // end of namespace cadence


TEST(P9_2000, testHeaderSize) {
    P9Protocol proc;

    ASSERT_EQ(4u + 1u + 2u, proc.headerSize());
}

TEST(P9_2000, testSettingFrameSize) {
    P9Protocol proc(127);

    ASSERT_EQ(127u, proc.maxPossibleMessageSize());
    ASSERT_EQ(127u, proc.maxNegotiatedMessageSize());

    proc.maxNegotiatedMessageSize(56);
    ASSERT_EQ(127u, proc.maxPossibleMessageSize());
    ASSERT_EQ(56u, proc.maxNegotiatedMessageSize());

    ASSERT_ANY_THROW(proc.maxNegotiatedMessageSize(300));
}

TEST(P9_2000, testParsingMessageHeader) {
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


TEST(P9_2000, parsingMessageHeaderWithUnknownMessageType) {
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

TEST(P9_2000, testParsingHeaderWithInsufficientData) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    ByteBuffer buffer(_mem.create(512));
    // Only write one header field. Should be not enough data to read a header.
    buffer << P9Protocol::size_type(4 + 1 + 2);
    auto res = proc.parseMessageHeader(buffer.flip());

    ASSERT_TRUE(res.isError());
}


TEST(P9_2000, testParsingIllformedMessageHeader) {
    MemoryManager _mem(1024);

    ByteBuffer buffer(_mem.create(512));
    // Set declared message size less then header size.
    buffer << P9Protocol::size_type(1 + 2);
    buffer << static_cast<byte>(P9Protocol::MessageType::TVersion);
    buffer << P9Protocol::Tag(1);

    P9Protocol proc;
    ASSERT_TRUE(proc.parseMessageHeader(buffer.flip()).isError());
}

TEST(P9_2000, parsingIllFormedHeaderForMessagesLargerMTUShouldError) {
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


TEST(P9_2000, parseIncorrectlySizedSmallerResponse) {
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

    auto message = proc.parseResponse(header.unwrap(), buffer);
    ASSERT_TRUE(message.isError());
}

TEST(P9_2000, parseIncorrectlySizedLargerResponse) {
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

    auto message = proc.parseResponse(header.unwrap(), buffer);
    ASSERT_TRUE(message.isError());
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 9P2000 Message parsing test suit
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class P9Messages : public ::testing::Test {
public:
    P9Messages() :
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





TEST_F(P9Messages, createVersionRequest) {
    const String testVersion = P9Protocol::PROTOCOL_VERSION;
    const uint32 versionStringLen = static_cast<uint32>(testVersion.size());

    P9Protocol proc;
    P9Protocol::RequestBuilder(_buffer)
            .version(testVersion);

    ASSERT_EQ(proc.headerSize() + 4 + 2 + versionStringLen, _buffer.position());

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TVersion, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(proc.maxPossibleMessageSize(), request.version.msize);
    ASSERT_STREQ(testVersion.c_str(), request.version.version.c_str());
}

TEST_F(P9Messages, createVersionRespose) {
    P9Protocol::ResponseBuilder(_buffer)
            .version("9Pe", 718);

    P9Protocol proc;
    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RVersion, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    ASSERT_EQ(718, response.version.msize);
    ASSERT_STREQ("9Pe", response.version.version.c_str());
}

TEST_F(P9Messages, parseVersionRespose) {
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + sizeof(int32) + sizeof(int16) + 2;
    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RVersion);
    _buffer << P9Protocol::Tag(1);
    _buffer << int32(512);
    _buffer << int16(2);
    _buffer.write("9P", 2);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RVersion, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();

    ASSERT_EQ(512, response.version.msize);
    ASSERT_STREQ("9P", response.version.version.c_str());
}


TEST_F(P9Messages, createAuthRequest) {
    P9Protocol proc;

    P9Protocol::RequestBuilder(_buffer)
        .auth(312, "User mcUsers", "Somewhere near");
    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TAuth, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(312, request.auth.afid);
    ASSERT_STREQ("User mcUsers", request.auth.uname.c_str());
    ASSERT_STREQ("Somewhere near", request.auth.aname.c_str());
}


TEST_F(P9Messages, createAuthRespose) {
    P9Protocol proc;

    P9Protocol::Qid qid;
    qid.path = 8187;
    qid.type = 71;
    qid.version = 17;
    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .auth(qid);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RAuth, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();

    ASSERT_EQ(qid, response.auth.qid);
}


TEST_F(P9Messages, parseAuthRespose) {
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + 13;
    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RAuth);
    _buffer << P9Protocol::Tag(1);

    _buffer << byte(13);     // QID.type
    _buffer << uint32(91);   // QID.version
    _buffer << uint64(441);  // QID.path

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RAuth, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(13, response.auth.qid.type);
    EXPECT_EQ(91, response.auth.qid.version);
    EXPECT_EQ(441, response.auth.qid.path);
}

// No such thing as error request!

TEST_F(P9Messages, createErrorRespose) {
    P9Protocol proc;

    const char* testError = "Something went right :)";
    P9Protocol::ResponseBuilder(_buffer)
            .tag(3)
            .error(testError);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RError, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isError());

    auto response = message.moveError();
    ASSERT_STREQ(testError, response.toString().c_str());
}

TEST_F(P9Messages, parseErrorRespose) {
    P9Protocol proc;

    const char* expectedErrorMessage = "All good!";
    const uint16 messageLen = static_cast<uint16>(strlen(expectedErrorMessage));
    const auto messageSize = proc.headerSize() + sizeof(uint16) + messageLen;

    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RError);
    _buffer << P9Protocol::Tag(1);

    _buffer << messageLen;
    _buffer.write(expectedErrorMessage, messageLen);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RError, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isError());

    auto response = message.moveError();
    EXPECT_STREQ(expectedErrorMessage, response.toString().c_str());
//    EXPECT_STREQ(expectedErrorMessage, response.error.ename.c_str());
}


TEST_F(P9Messages, createFlushRequest) {
    P9Protocol proc;

    P9Protocol::RequestBuilder(_buffer)
            .flush(7711);
    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TFlush, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(7711, request.flush.oldtag);
}


TEST_F(P9Messages, createFlushRespose) {
    P9Protocol proc;

    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .flush();

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RFlush, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());
}


TEST_F(P9Messages, parseFlushRespose) {
    P9Protocol proc;

    const auto messageSize = proc.headerSize();
    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RFlush);
    _buffer << P9Protocol::Tag(1);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RFlush, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());
}


TEST_F(P9Messages, createAttachRequest) {
    P9Protocol proc;

    P9Protocol::RequestBuilder(_buffer)
            .attach(3310, 1841, "McFace", "close to u");
    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TAttach, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(3310, request.attach.fid);
    ASSERT_EQ(1841, request.attach.afid);
    ASSERT_STREQ("McFace", request.attach.uname.c_str());
    ASSERT_STREQ("close to u", request.attach.aname.c_str());
}

TEST_F(P9Messages, createAttachRespose) {
    P9Protocol proc;

    P9Protocol::Qid qid;
    qid.path = 7771;
    qid.type = 91;
    qid.version = 3;
    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .attach(qid);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RAttach, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    ASSERT_EQ(qid, response.attach.qid);
}

TEST_F(P9Messages, parseAttachRespose) {
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + 13;
    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RAttach);
    _buffer << P9Protocol::Tag(1);

    _buffer << byte(81);     // QID.type
    _buffer << uint32(3);   // QID.version
    _buffer << uint64(1049);  // QID.path

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RAttach, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(81, response.attach.qid.type);
    EXPECT_EQ(3, response.attach.qid.version);
    EXPECT_EQ(1049, response.attach.qid.path);
}


TEST_F(P9Messages, createOpenRequest) {
    P9Protocol proc;

    P9Protocol::RequestBuilder(_buffer)
            .open(517, 11);
    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TOpen, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(517, request.open.fid);
    ASSERT_EQ(11, request.open.mode);
}


TEST_F(P9Messages, createOpenRespose) {
    P9Protocol proc;

    P9Protocol::Qid qid;
    qid.path = 323;
    qid.type = 8;
    qid.version = 13;
    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .open(qid, 817);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::ROpen, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();

    ASSERT_EQ(qid, response.open.qid);
    ASSERT_EQ(817, response.open.iounit);
}

TEST_F(P9Messages, parseOpenRespose) {
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + 13 + sizeof(uint32);
    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::ROpen);
    _buffer << P9Protocol::Tag(1);
    // qid
    _buffer << byte(71);     // QID.type
    _buffer << uint32(33);   // QID.version
    _buffer << uint64(4173);  // QID.path
    // iounit
    _buffer << uint32(998);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::ROpen, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(71, response.open.qid.type);
    EXPECT_EQ(33, response.open.qid.version);
    EXPECT_EQ(4173, response.open.qid.path);
    EXPECT_EQ(998, response.open.iounit);
}



TEST_F(P9Messages, createCreateRequest) {
    P9Protocol proc;

    P9Protocol::RequestBuilder(_buffer)
            .create(1734, "mcFance", 11, 077);
    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TCreate, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(1734, request.create.fid);
    ASSERT_STREQ("mcFance", request.create.name.c_str());
    ASSERT_EQ(11, request.create.perm);
    ASSERT_EQ(077, request.create.mode);
}

TEST_F(P9Messages, createCreateRespose) {
    P9Protocol proc;

    P9Protocol::Qid qid;
    qid.path = 323;
    qid.type = 8;
    qid.version = 13;
    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .create(qid, 718);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RCreate, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();

    ASSERT_EQ(qid, response.create.qid);
    ASSERT_EQ(718, response.create.iounit);
}

TEST_F(P9Messages, parseCreateRespose) {
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + 13 + sizeof(uint32);
    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RCreate);
    _buffer << P9Protocol::Tag(1);
    // qid
    _buffer << byte(87);     // QID.type
    _buffer << uint32(5481);   // QID.version
    _buffer << uint64(17);  // QID.path
    // iounit
    _buffer << uint32(778);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RCreate, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(87, response.create.qid.type);
    EXPECT_EQ(5481, response.create.qid.version);
    EXPECT_EQ(17, response.create.qid.path);
    EXPECT_EQ(778, response.create.iounit);
}


TEST_F(P9Messages, createReadRequest) {
    P9Protocol proc;

    P9Protocol::RequestBuilder(_buffer)
            .read(7234, 18, 772);
    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TRead, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(7234, request.read.fid);
    ASSERT_EQ(18, request.read.offset);
    ASSERT_EQ(772, request.read.count);
}

TEST_F(P9Messages, createReadRespose) {
    P9Protocol proc;

    const char content[] = "Good news everyone!";
    auto data = wrapMemory(content);
    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .read(data);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RRead, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();

    ASSERT_EQ(data, response.read.data);
}

TEST_F(P9Messages, parseReadRespose) {
    P9Protocol proc;

    const char* messageData = "This is a very important data d-_^b";
    const uint32 dataLen = static_cast<uint32>(strlen(messageData));
    const auto messageSize = proc.headerSize() + sizeof(uint32) + dataLen;

    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RRead);
    _buffer << P9Protocol::Tag(1);
    // iounit
    _buffer << dataLen;
    _buffer.write(messageData, dataLen);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RRead, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(dataLen, response.read.data.size());
    EXPECT_EQ(0, memcmp(response.read.data.dataAddress(), messageData, dataLen));
}


TEST_F(P9Messages, createWriteRequest) {
    P9Protocol proc;

    const char messageData[] = "This is a very important data d-_^b";
    auto data = wrapMemory(messageData);

    P9Protocol::RequestBuilder(_buffer)
            .write(15927, 98, data);
    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TWrite, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(15927, request.write.fid);
    ASSERT_EQ(98, request.write.offset);
    ASSERT_EQ(data, request.write.data);
}

TEST_F(P9Messages, createWriteRespose) {
    MemoryManager _mem(1024);
    P9Protocol proc;

    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .write(71717);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RWrite, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();

    ASSERT_EQ(71717, response.write.count);
}

TEST_F(P9Messages, parseWriteRespose) {
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + sizeof(uint32);
    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RWrite);
    _buffer << P9Protocol::Tag(1);
    // iounit
    _buffer << uint32(81177);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RWrite, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(81177, response.write.count);
}



TEST_F(P9Messages, createClunkRequest) {
    P9Protocol proc;

    P9Protocol::RequestBuilder(_buffer)
            .clunk(37509);
    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TClunk, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(37509, request.clunk.fid);
}

TEST_F(P9Messages, createClunkRespose) {
    P9Protocol proc;
    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .clunk();

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RClunk, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());
}

TEST_F(P9Messages, parseClunkRespose) {
    P9Protocol proc;

    const auto messageSize = proc.headerSize();
    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RClunk);
    _buffer << P9Protocol::Tag(1);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RClunk, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());
}



TEST_F(P9Messages, createRemoveRequest) {
    P9Protocol proc;

    P9Protocol::RequestBuilder(_buffer)
            .remove(54329);
    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TRemove, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(54329, request.remove.fid);
}

TEST_F(P9Messages, createRemoveRespose) {
    P9Protocol proc;
    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .remove();

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RRemove, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());
}

TEST_F(P9Messages, parseRemoveRespose) {
    P9Protocol proc;

    const auto messageSize = proc.headerSize();
    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RRemove);
    _buffer << P9Protocol::Tag(1);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RRemove, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());
}



TEST_F(P9Messages, createStatRequest) {
    P9Protocol proc;

    P9Protocol::RequestBuilder(_buffer)
            .stat(7872);
    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TStat, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(7872, request.stat.fid);
}

TEST_F(P9Messages, createStatRespose) {
    P9Protocol proc;

    P9Protocol::Stat stat;
    stat.atime = 12;
    stat.dev = 3310;
    stat.gid = "Nice user";
    stat.length = 414;
    stat.mode = 111;
    stat.mtime = 17;
    stat.name = "File McFileface";
    stat.qid.path = 68171;
    stat.qid.type = 7;
    stat.qid.version = 4;
    stat.size = 124;
    stat.type = 3;
    stat.uid = "User McUserface -2";

    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .stat(stat);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RStat, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();

    ASSERT_EQ(stat, response.stat);
}

TEST_F(P9Messages, parseStatRespose) {
    P9Protocol proc;

    // Set declared message size to be more then negotiated message size
    const auto headPosition = _buffer.position();
    _buffer << P9Protocol::size_type(0);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RStat);
    _buffer << P9Protocol::Tag(1);

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

    encode9P(_buffer, stat);
    const auto totalSize = _buffer.position();
    _buffer.reset(headPosition) << P9Protocol::size_type(totalSize);
    _buffer.reset(totalSize);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RStat, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(stat, response.stat);
}


TEST_F(P9Messages, createWStatRequest) {
    P9Protocol proc;

    P9Protocol::Stat stat;
    stat.atime = 21;
    stat.dev = 8828;
    stat.gid = "Other user";
    stat.length = 818177;
    stat.mode = 111;
    stat.mtime = 17;
    stat.name = "la-la McFile";
    stat.qid.path = 61;
    stat.qid.type = 15;
    stat.qid.version = 404;
    stat.size = 124;
    stat.type = 1;
    stat.uid = "Userface McUse";

    P9Protocol::RequestBuilder(_buffer)
            .writeStat(8193, stat);
    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TWStat, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(8193, request.wstat.fid);
    ASSERT_EQ(stat, request.wstat.stat);
}

TEST_F(P9Messages, createWStatRespose) {
    P9Protocol proc;

    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .wstat();

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RWStat, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());
}

TEST_F(P9Messages, parseWStatRespose) {
    P9Protocol proc;

    const auto messageSize = proc.headerSize();
    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RWStat);
    _buffer << P9Protocol::Tag(1);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RWStat, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 9P2000.e
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST_F(P9Messages, createSessionRequest) {
    const byte sessionKey[8] = {8, 7, 6, 5, 4, 3, 2, 1};
    const auto data = wrapMemory(sessionKey);

    P9Protocol proc;
    P9Protocol::RequestBuilder(_buffer)
            .session(data);

    ASSERT_EQ(proc.headerSize() + 8, _buffer.position());

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TSession, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(data, request.session.key);
}

TEST_F(P9Messages, createSessionRespose) {
    P9Protocol proc;

    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .session();

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RSession, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());
}

TEST_F(P9Messages, parseSessionRespose) {
    P9Protocol proc;

    const auto messageSize = proc.headerSize();
    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RSession);
    _buffer << P9Protocol::Tag(1);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RSession, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());
}



TEST_F(P9Messages, createShortReadRequest) {
    const auto path = Path::parse("some/wierd/place");

    P9Protocol proc;
    P9Protocol::RequestBuilder(_buffer)
            .shortRead(32, path);

    ASSERT_EQ(proc.headerSize() + sizeof(P9Protocol::Fid) + sizeof (uint16)
              + 3*sizeof (uint16) + (4 + 5 + 5), _buffer.position());

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TSRead, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(32, request.shortRead.fid);
    ASSERT_EQ(path, request.shortRead.path);
}


TEST_F(P9Messages, createShortReadRespose) {
    P9Protocol proc;

    const char messageData[] = "This was somewhat important data d^_-b";
    auto data = wrapMemory(messageData);
    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .shortRead(data);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RSRead, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(data, response.read.data);
}


TEST_F(P9Messages, parseShortReadRespose) {
    P9Protocol proc;

    const char* messageData = "This is a very important data d-_^b";
    const uint32 dataLen = static_cast<uint32>(strlen(messageData));
    const auto messageSize = proc.headerSize() + sizeof(uint32) + dataLen;

    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RSRead);
    _buffer << P9Protocol::Tag(1);
    // iounit
    _buffer << dataLen;
    _buffer.write(messageData, dataLen);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RSRead, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(dataLen, response.read.data.size());
    EXPECT_EQ(0, memcmp(response.read.data.dataAddress(), messageData, dataLen));
}


TEST_F(P9Messages, createShortWriteRequest) {
    const auto path = Path::parse("some/wierd/place");
    const char messageData[] = "This is a very important data d-_^b";
    auto data = wrapMemory(messageData);

    P9Protocol proc;
    P9Protocol::RequestBuilder(_buffer)
            .shortWrite(32, path, data);

    ASSERT_EQ(proc.headerSize() + sizeof(P9Protocol::Fid) + sizeof (uint16)
              + 3*sizeof (uint16) + (4 + 5 + 5) +
              sizeof(P9Protocol::size_type) + data.size(), _buffer.position());

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::TSWrite, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseRequest(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto request = message.moveResult();
    ASSERT_EQ(32, request.shortWrite.fid);
    ASSERT_EQ(path, request.shortWrite.path);
    ASSERT_EQ(data, request.shortWrite.data);
}


TEST_F(P9Messages, createShortWriteRespose) {
    P9Protocol proc;

    P9Protocol::ResponseBuilder(_buffer)
            .tag(1)
            .shortWrite(100500);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RSWrite, header.type);

    // Make sure we can parse the message back.
    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(100500, response.write.count);
}


TEST_F(P9Messages, parseShortWriteRespose) {
    P9Protocol proc;

    const auto messageSize = proc.headerSize() + sizeof(uint32);
    // Set declared message size to be more then negotiated message size
    _buffer << P9Protocol::size_type(messageSize);
    _buffer << static_cast<byte>(P9Protocol::MessageType::RSWrite);
    _buffer << P9Protocol::Tag(1);
    // iounit
    _buffer << uint32(81177);

    auto headerResult = proc.parseMessageHeader(_buffer.flip());
    ASSERT_TRUE(headerResult.isOk());

    auto header = headerResult.unwrap();
    ASSERT_EQ(P9Protocol::MessageType::RSWrite, header.type);

    auto message = proc.parseResponse(header, _buffer);
    ASSERT_TRUE(message.isOk());

    auto response = message.moveResult();
    EXPECT_EQ(81177, response.write.count);
}
