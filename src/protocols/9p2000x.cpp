/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/

#include "cadence/protocols/9p2000x.hpp"

#include <solace/assert.hpp>
#include <solace/exception.hpp>

#include <limits>
#include <cstring>



using namespace Solace;
using namespace cadence;



const P9Protocol::size_type P9Protocol::MAX_MESSAGE_SIZE = 4*1024;  // 4k should be enough for everyone, am I right?
const String                P9Protocol::PROTOCOL_VERSION = "9P2000.x";
const P9Protocol::Tag       P9Protocol::NO_TAG = static_cast<P9Protocol::Tag>(~0);
const P9Protocol::fid_type  P9Protocol::NOFID = static_cast<P9Protocol::fid_type>(~0);


static const char* UNKNOWN_PROTOCOL_VERSION = "unknown";



uint16 protocolStringSize(const String& str) {
    return sizeof(uint16) +                     // String size var
            static_cast<uint16>(str.size());    // Space for the actual string bytes
}


uint16 protocolPathSize(const Path& path) {
    uint16 stringsSize = 0;
    for (auto& segment : path) {
        stringsSize += protocolStringSize(segment);
    }

    return sizeof(uint16) +  // Var number of segments
            stringsSize;
}


void writeMessageHeader(ByteBuffer& dest,
                        P9Protocol::size_type messageSize,
                        P9Protocol::MessageType type,
                        P9Protocol::Tag tag) {
    dest << messageSize;
    dest << static_cast<byte>(type);
    dest << tag;
}

void writeString(ByteBuffer& dest, const char* str) {
    const auto dataSize = static_cast<uint16>(strnlen(str, std::numeric_limits<Solace::uint16>::max()));
    dest << dataSize;
    dest.write(str, dataSize);
}

void writeString(ByteBuffer& dest, const String& str) {
    writeString(dest, str.c_str());
}

String readString(ByteBuffer& src) {
    uint16 dataSize = 0;
    src >> dataSize;

    std::string buff;
    buff.reserve(dataSize);

    for (decltype(dataSize) i = 0; i < dataSize; ++i) {
        char c;
        src >> c;

        buff.append(1, c);
    }

    return buff;
}


void readQid(ByteBuffer& data, P9Protocol::Qid* qid) {
    data >> qid->type;
    data >> qid->version;
    data >> qid->path;
}


void writeQid(ByteBuffer& data, const P9Protocol::Qid& qid) {
    data << qid.type;
    data << qid.version;
    data << qid.path;
}


void writeStat(ByteBuffer& dest, const P9Protocol::Stat& stat) {
    dest << stat.size;
    dest << stat.type;
    dest << stat.dev;

    // Write Qid
    writeQid(dest, stat.qid);

    dest << stat.mode;
    dest << stat.atime;
    dest << stat.mtime;
    dest << stat.length;

    writeString(dest, stat.name);
    writeString(dest, stat.uid);
    writeString(dest, stat.gid);
    writeString(dest, stat.muid);
}


void readStat(ByteBuffer& src, P9Protocol::Stat* stat) {
    src >> stat->size;
    src >> stat->type;
    src >> stat->dev;

    readQid(src, &(stat->qid));

    src >> stat->mode;
    src >> stat->atime;
    src >> stat->mtime;
    src >> stat->length;

    stat->name = readString(src);
    stat->uid = readString(src);
    stat->gid = readString(src);
    stat->muid = readString(src);
}


P9Protocol::fid_type P9Protocol::allocateFid() {
    return 2 * _currentTag + 1;  // FIXME: It is really WTF!
}


P9Protocol::Tag P9Protocol::createVersionRequest(Tag, ByteBuffer& dest, const String& version) {

    // Compute message size first:
    const size_type messageSize = headerSize() +
            sizeof(Solace::uint32) +                // Negotiated message size field
            protocolStringSize(version);   // Version string data

    writeMessageHeader(dest, messageSize, MessageType::TVersion, NO_TAG);

    dest << maxPossibleMessageSize();
    writeString(dest, version);

    return NO_TAG;
}


P9Protocol::Tag
P9Protocol::createAuthRequest(Tag tag, ByteBuffer& dest, fid_type afid,
                              const String& userName, const String& attachName) {
    // Compute message size first:
    const size_type messageSize = headerSize() +
            sizeof(fid_type) +                  // Proposed fid for authentication mechanism
            protocolStringSize(userName);       // User name
            protocolStringSize(attachName);     // Root name where we want to attach to

    writeMessageHeader(dest, messageSize, MessageType::TAuth, tag);
    dest << afid;
    writeString(dest, userName);
    writeString(dest, attachName);

    return tag;
}

P9Protocol::Tag
P9Protocol::createAttachRequest(Tag tag, ByteBuffer& dest,
                                      fid_type fid, fid_type afid,
                                      const String& userName, const String& attachName) {
    // Compute message size first:
    const size_type messageSize = headerSize() +
            sizeof(fid_type) +                  // Proposed fid for the attached root
            sizeof(fid_type) +                  // Fid of the passed authentication
            protocolStringSize(userName) +      // User name
            protocolStringSize(attachName);     // Root name where we want to attach to

    writeMessageHeader(dest, messageSize, MessageType::TAttach, tag);
    dest << fid;
    dest << afid;
    writeString(dest, userName);
    writeString(dest, attachName);

    return tag;
}

P9Protocol::Tag
P9Protocol::createClunkRequest(Tag tag, Solace::ByteBuffer& dest, fid_type fid) {
    // Compute message size first:
    const size_type messageSize = headerSize() +
            sizeof(fid_type);               // Fid to forget

    writeMessageHeader(dest, messageSize, MessageType::TClunk, tag);
    dest << fid;

    return tag;
}


P9Protocol::Tag
P9Protocol::createFlushRequest(Tag tag, Solace::ByteBuffer& dest, Tag oldTransation) {
    // Compute message size first:
    const size_type messageSize = headerSize() +
            sizeof(Tag);               // Fid to forget

    writeMessageHeader(dest, messageSize, MessageType::TFlush, tag);
    dest << oldTransation;

    return tag;
}


P9Protocol::Tag
P9Protocol::createRemoveRequest(Tag tag, Solace::ByteBuffer& dest, fid_type fid) {
    // Compute message size first:
    const size_type messageSize = headerSize() +
            sizeof(fid_type);               // Fid to forget

    writeMessageHeader(dest, messageSize, MessageType::TRemove, tag);
    dest << fid;

    return tag;
}


P9Protocol::Tag
P9Protocol::createOpenRequest(Tag tag, ByteBuffer& dest, fid_type fid, byte mode) {
    // Compute message size first:
    const size_type messageSize = headerSize() +
            sizeof(fid) +                // Fid of the file to open
            sizeof(mode);                // Mode of the file to open

    writeMessageHeader(dest, messageSize, MessageType::TOpen, tag);
    dest << fid;
    dest << mode;

    return tag;
}


P9Protocol::Tag
P9Protocol::createCreateRequest(Tag tag, ByteBuffer& dest,
                                       fid_type fid,
                                       const String& name,
                                       uint32 permissions,
                                       byte mode) {
    // Compute message size first:
    const size_type messageSize = headerSize() +
            sizeof(fid) +
            protocolStringSize(name) +
            sizeof(permissions) +
            sizeof(mode);

    writeMessageHeader(dest, messageSize, MessageType::TCreate, tag);
    dest << fid;
    writeString(dest, name);
    dest << permissions;
    dest << mode;

    return tag;
}


P9Protocol::Tag
P9Protocol::createReadRequest(Tag tag, ByteBuffer& dest,
                              fid_type fid, uint64 offset, size_type count) {
    // Compute message size first:
    const size_type messageSize = headerSize() +
            sizeof(fid) +
            sizeof(offset) +
            sizeof(count);

    writeMessageHeader(dest, messageSize, MessageType::TRead, tag);
    dest << fid;
    dest << offset;
    dest << count;

    return tag;
}


P9Protocol::Tag
P9Protocol::createWriteRequest(Tag tag, ByteBuffer& dest,
                               fid_type fid, uint64 offset, size_type count, const byte* data) {
    // Compute message size first:
    const size_type messageSize = headerSize() +
            sizeof(fid) +
            sizeof(offset) +
            sizeof(count) +
            count;

    writeMessageHeader(dest, messageSize, MessageType::TWrite, tag);
    dest << fid;
    dest << offset;
    dest << count;
    dest.write(data, count);

    return tag;
}


P9Protocol::Tag
P9Protocol::createWalkRequest(Tag tag, ByteBuffer& dest,
                              fid_type fid, fid_type nfid, const Path& path) {
    // Compute message size first:
    const size_type messageSize = headerSize() +
            sizeof(fid) +
            sizeof(nfid) +
            protocolPathSize(path);

    writeMessageHeader(dest, messageSize, MessageType::TWalk, tag);
    dest << fid;
    dest << nfid;
    dest << static_cast<uint16>(path.getComponentsCount());

    for (const auto& component : path){
        writeString(dest, component);
    }

    return tag;
}


P9Protocol::Tag
P9Protocol::createStatRequest(Tag tag, Solace::ByteBuffer& dest,
                              fid_type fid) {
    // Compute message size first:
    const size_type messageSize = headerSize() +
            sizeof(fid);

    writeMessageHeader(dest, messageSize, MessageType::TStat, tag);
    dest << fid;

    return tag;
}


P9Protocol::Tag
P9Protocol::createWriteStatRequest(Tag tag, Solace::ByteBuffer& dest,
                                   fid_type fid, const Stat& stat) {
    // Compute message size first:
    const size_type messageSize = headerSize() +
            sizeof(fid) +
            stat.size;

    writeMessageHeader(dest, messageSize, MessageType::TWStat, tag);
    dest << fid;
    writeStat(dest, stat);

    return tag;
}


Result<P9Protocol::Response, Error>
P9Protocol::parseErrorResponse(const MessageHeader& SOLACE_UNUSED(header), ByteBuffer& buffer) const {
    return Err(Error(readString(buffer).to_str()));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseVersionResponse(const MessageHeader& header, ByteBuffer& data) const {
    uint32 newMaxMessageSize = 0;
    data >> newMaxMessageSize;

    Response fcall(header.type, header.tag);

    fcall.version.msize = newMaxMessageSize;
    // FIXME: As wrong as it gets!
    fcall.version.version = readString(data);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseAuthResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    readQid(data, &fcall.auth.qid);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseAttachResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    readQid(data, &fcall.attach.qid);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseClunkResponse(const MessageHeader& header, ByteBuffer& SOLACE_UNUSED(data)) const {
    Response fcall(header.type, header.tag);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseFlushResponse(const MessageHeader& header, ByteBuffer& SOLACE_UNUSED(data)) const {
    Response fcall(header.type, header.tag);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseOpenResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    readQid(data, &fcall.open.qid);
    data >> fcall.open.iounit;

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseCreateResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    readQid(data, &fcall.create.qid);
    data >> fcall.open.iounit;

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseReadResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    uint32 bytesRead;
    data >> bytesRead;
    fcall.read.data = data.viewRemaining();

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseWriteResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    data >> fcall.write.count;

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseRemoveResponse(const MessageHeader& header, ByteBuffer& SOLACE_UNUSED(data)) const {
    Response fcall(header.type, header.tag);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseStatResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    readStat(data, &(fcall.stat));

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseWStatResponse(const MessageHeader& header, ByteBuffer& SOLACE_UNUSED(data)) const {
    Response fcall(header.type, header.tag);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseWalkResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    data >> fcall.walk.nqids;
    for (decltype(fcall.walk.nqids) i = 0; i < fcall.walk.nqids; ++i) {
        readQid(data, &fcall.walk.qids[i]);
    }

    return Ok(std::move(fcall));
}



Result<P9Protocol::MessageHeader, Error>
P9Protocol::parseMessageHeader(ByteBuffer& buffer) const {
//    Solace::assertIndexInRange(buffer.viewWritten().size(), 0, headerSize());
    const auto mandatoryHeaderSize = headerSize();
    const auto dataAvailliable = buffer.remaining();
    if (dataAvailliable < mandatoryHeaderSize)
        return Err(Error("Ill-formed message header. Not enough data to read a header"));

    MessageHeader header;
    buffer >> header.size;

    // Sanity checks:
    // It is a serious error if server responded with the message of a size bigger than negotiated one.
//    Solace::assertIndexInRange(header.size, headerSize(), maxNegotiatedMessageSize());
    if (header.size < headerSize())
        return Err(Error("Ill-formed message: Declared frame size less than header"));
    if (header.size > maxNegotiatedMessageSize())
        return Err(Error("Ill-formed message: Declared frame size greater than negotiated message size"));

    // Read message type:
    byte messageBytecode;
    buffer >> messageBytecode;
    // don't want any funny messages.
//    Solace::assertIndexInRange(messageBytecode,
//                               static_cast<byte>(MessageType::_beginSupportedMessageCode),
//                               static_cast<byte>(MessageType::_endSupportedMessageCode));
    header.type = static_cast<MessageType>(messageBytecode);
    if (header.type < MessageType::_beginSupportedMessageCode ||
        header.type >= MessageType::_endSupportedMessageCode)
        return Err(Error("Ill-formed message: Unsupported message type"));

    // Read message tag. Tags are provided by the client and can not be checked by the message parser.
    // Unless we are provided with the expected tag...
    buffer >> header.tag;

    return Ok(header);
}


Result<P9Protocol::Response, Error>
P9Protocol::parseMessage(const MessageHeader& header, ByteBuffer& data) const {
    const auto expectedData = header.size - headerSize();

    // Message data sanity check
    // Make sure we have been given enough data to read a message as requested in the message size.
    if (expectedData > data.remaining())
        return Err(Error("Ill-formed message: Declared frame size larger than message data received"));

    // Make sure there is no extra data in the buffer.
    if (expectedData < data.remaining())
        return Err(Error("Ill-formed message: Declared frame size less than message data received"));

    switch (header.type) {
    case MessageType::RError:   return parseErrorResponse(header,   data);
    case MessageType::RVersion: return parseVersionResponse(header, data);
    case MessageType::RAuth:    return parseAuthResponse(header,    data);
    case MessageType::RAttach:  return parseAttachResponse(header,  data);
    case MessageType::RFlush:   return parseFlushResponse(header,   data);
    case MessageType::RWalk:    return parseWalkResponse(header,    data);
    case MessageType::ROpen:    return parseOpenResponse(header,    data);
    case MessageType::RCreate:  return parseCreateResponse(header,  data);
    case MessageType::RRead:    return parseReadResponse(header,    data);
    case MessageType::RWrite:   return parseWriteResponse(header,   data);
    case MessageType::RClunk:   return parseClunkResponse(header,   data);
    case MessageType::RRemove:  return parseRemoveResponse(header,  data);
    case MessageType::RStat:    return parseStatResponse(header,    data);
    case MessageType::RWStat:   return parseWStatResponse(header,   data);
    default:
        return Err(Error("Failed to parse responce message: Unsupported message type"));
    }
}


P9Protocol::size_type P9Protocol::maxNegotiatedMessageSize(size_type newMessageSize) {
    Solace::assertIndexInRange(newMessageSize, 0, maxPossibleMessageSize());
    _maxNegotiatedMessageSize = std::min(newMessageSize, maxPossibleMessageSize());

    return _maxNegotiatedMessageSize;
}


P9Protocol::P9Protocol(size_type maxMassageSize) :
    _maxMassageSize(maxMassageSize),
    _maxNegotiatedMessageSize(maxMassageSize),
    _currentTag(1)
{

}


P9Protocol::Response::Response(MessageType msgType, Tag msgTag) :
    type(msgType),
    tag(msgTag)
{
    switch (type) {
    case MessageType::RError:   new (&error) Error;     return;
    case MessageType::RVersion: new (&version) Version; return;
    case MessageType::RAuth:    new (&auth) Auth;       return;
    case MessageType::RAttach:  new (&attach) Attach;   return;
    case MessageType::RWalk:    new (&walk) Walk;       return;
    case MessageType::ROpen:    new (&open) Open;       return;
    case MessageType::RCreate:  new (&create) Create;   return;
    case MessageType::RRead:    new (&read) Read;       return;
    case MessageType::RWrite:   new (&write) Write;     return;
    case MessageType::RStat:    new (&stat) Stat;       return;
    case MessageType::RClunk:
    case MessageType::RRemove:
    case MessageType::RFlush:
    case MessageType::RWStat:
        break;

    default:
        Solace::raise<IOException>("Unexpected message type");
        break;
    }
}


P9Protocol::Response::Response(Response&& rhs) :
    type(rhs.type),
    tag(rhs.tag)
{
    switch (type) {
    case MessageType::RError:   new (&error) Error(std::move(rhs.error)); return;
    case MessageType::RVersion: new (&version) Version(std::move(rhs.version)); return;
    case MessageType::RAuth:    new (&auth) Auth(std::move(rhs.auth)); return;
    case MessageType::RAttach:  new (&attach) Attach(std::move(rhs.attach)); return;
    case MessageType::RWalk:    new (&walk) Walk(std::move(rhs.walk)); return;
    case MessageType::ROpen:    new (&open) Open(std::move(rhs.open)); return;
    case MessageType::RCreate:  new (&create) Create(std::move(rhs.create)); return;
    case MessageType::RRead:    new (&read) Read(std::move(rhs.read)); return;
    case MessageType::RWrite:   new (&write) Write(std::move(rhs.write)); return;
    case MessageType::RStat:    new (&stat) Stat(std::move(rhs.stat)); return;
    case MessageType::RClunk:
    case MessageType::RRemove:
    case MessageType::RFlush:
    case MessageType::RWStat:
        break;

    default:
        Solace::raise<IOException>("Unexpected message type");
        break;
    }
}


P9Protocol::Response::~Response() {
    switch (type) {
    case MessageType::RError:   (&error)->~Error();     break;
    case MessageType::RVersion: (&version)->~Version(); break;
    case MessageType::RAuth:    (&auth)->~Auth();       break;
    case MessageType::RAttach:  (&attach)->~Attach();   break;
    case MessageType::RWalk:    (&walk)->~Walk();       break;
    case MessageType::ROpen:    (&open)->~Open();       break;
    case MessageType::RCreate:  (&create)->~Create();   break;
    case MessageType::RRead:    (&read)->~Read();       break;
    case MessageType::RWrite:   (&write)->~Write();     break;
    case MessageType::RStat:    (&stat)->~Stat();       break;
    case MessageType::RClunk:
    case MessageType::RRemove:
    case MessageType::RFlush:
    case MessageType::RWStat:
    default:
        break;
    }
}

