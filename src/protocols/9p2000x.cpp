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



const P9Protocol::size_type P9Protocol::MAX_MESSAGE_SIZE = 4*1024;      // 4k should be enough for everyone, am I right?
const String                P9Protocol::PROTOCOL_VERSION = "9P2000.e";  // By default we want to talk via 9P2000.e proc
const P9Protocol::Tag       P9Protocol::NO_TAG = static_cast<P9Protocol::Tag>(~0);
const P9Protocol::fid_type  P9Protocol::NOFID = static_cast<P9Protocol::fid_type>(~0);


// static const char* UNKNOWN_PROTOCOL_VERSION = "unknown";

P9Protocol::size_type protocolSize(const uint8& value) {
    return sizeof(value);
}

P9Protocol::size_type protocolSize(const uint16& value) {
    return sizeof(value);
}

P9Protocol::size_type protocolSize(const uint32& value) {
    return sizeof(value);
}

P9Protocol::size_type protocolSize(const uint64& value) {
    return sizeof(value);
}

P9Protocol::size_type protocolSize(const String& str) {
    return sizeof(uint16) +                     // String size var
            static_cast<uint16>(str.size());    // Space for the actual string bytes
}


P9Protocol::size_type protocolSize(const Path& path) {
    P9Protocol::size_type payloadSize = 0;
    for (auto& segment : path) {
        payloadSize += protocolSize(segment);
    }

    return sizeof(uint16) +  // Var number of segments
            payloadSize;
}


P9Protocol::size_type protocolSize(const P9Protocol::Qid&) {
    static constexpr P9Protocol::size_type kQidSize = sizeof(P9Protocol::Qid::type) +
            sizeof(P9Protocol::Qid::version) +
            sizeof(P9Protocol::Qid::path);

    static_assert(kQidSize == 13, "Incorrect Qid struct size");

    return kQidSize;
}


P9Protocol::size_type protocolSize(const P9Protocol::Stat& stat) {
    return  protocolSize(stat.size) +
            protocolSize(stat.type) +
            protocolSize(stat.dev) +
            protocolSize(stat.qid) +
            protocolSize(stat.mode) +
            protocolSize(stat.atime) +
            protocolSize(stat.mtime) +
            protocolSize(stat.length) +
            protocolSize(stat.name) +
            protocolSize(stat.uid) +
            protocolSize(stat.gid) +
            protocolSize(stat.muid);
}


P9Protocol::size_type protocolSize(const Array<P9Protocol::Qid>& qids) {
    P9Protocol::Qid uselessQid;

    return sizeof(uint16) +  // Var number of elements
            qids.size() * protocolSize(uselessQid);
}


P9Protocol::size_type protocolSize(const ImmutableMemoryView& data) {
    return sizeof(P9Protocol::size_type) +  // Var number of elements
            data.size();
}


/**
 * P9 protocol helper class to encode data into a protocol message format.
 */
class P9Encoder {
public:

    P9Encoder(ByteBuffer& dest) :
        _dest(dest)
    {}

    P9Encoder& header(P9Protocol::MessageType type, P9Protocol::Tag tag, P9Protocol::size_type payloadSize = 0) {
        return encode(P9Protocol::headerSize() + payloadSize)
                .encode(static_cast<byte>(type))
                .encode(tag);
    }


    P9Encoder& encode(uint8 value) { _dest << value; return (*this); }

    P9Encoder& encode(uint16 value) {
        _dest << value;
        return (*this);
    }

    P9Encoder& encode(uint32 value) {
        _dest << value;
        return (*this);
    }

    P9Encoder& encode(uint64 value) {
        _dest << value;
        return (*this);
    }

    P9Encoder& encode(const char* str) {
        const auto dataSize = static_cast<uint16>(strnlen(str, std::numeric_limits<Solace::uint16>::max()));
        encode(dataSize);
        _dest.write(str, dataSize);

        return (*this);
    }

    P9Encoder& encode(const String& str) {
        return encode(str.c_str());
    }

    P9Encoder& encode(const P9Protocol::Qid& qid) {
        return encode(qid.type)
                .encode(qid.version)
                .encode(qid.path);
    }

    P9Encoder& encode(const Array<P9Protocol::Qid>& qids) {
        encode(static_cast<uint16>(qids.size()));
        for (auto qid : qids) {
            encode(qid);
        }

        return (*this);
    }

    P9Encoder& encode(const P9Protocol::Stat& stat) {
        return encode(stat.size)
                .encode(stat.type)
                .encode(stat.dev)
                .encode(stat.qid)
                .encode(stat.mode)
                .encode(stat.atime)
                .encode(stat.mtime)
                .encode(stat.length)
                .encode(stat.name)
                .encode(stat.uid)
                .encode(stat.gid)
                .encode(stat.muid);
    }

    P9Encoder& encode(const ImmutableMemoryView& data) {
        encode(static_cast<P9Protocol::size_type>(data.size()));
        _dest.write(data);

        return (*this);
    }

    P9Encoder& encode(const Path& path) {
        encode(static_cast<uint16>(path.getComponentsCount()));

        for (const auto& component : path) {
            encode(component);
        }

        return (*this);
    }

private:
    ByteBuffer& _dest;
};


class P9Decoder {
public:

    P9Decoder(ByteBuffer& src) :
        _src(src)
    {}

    P9Decoder& read(uint8* dest) {
        _src >> *dest;
        return *this;
    }

    P9Decoder& read(uint16* dest) {
        _src >> *dest;
        return *this;
    }

    P9Decoder& read(uint32* dest) {
        _src >> *dest;
        return *this;
    }

    P9Decoder& read(uint64* dest) {
        _src >> *dest;
        return *this;
    }

    P9Decoder& read(String* dest) {
        uint16 dataSize = 0;
        _src >> dataSize;

        std::string buff;
        buff.reserve(dataSize);

        for (decltype(dataSize) i = 0; i < dataSize; ++i) {
            char c;
            _src >> c;

            buff.append(1, c);
        }

        *dest = buff;

        return *this;
    }

    P9Decoder& read(P9Protocol::Qid* qid) {
        return read(&qid->type)
                .read(&qid->version)
                .read(&qid->path);
    }


    P9Decoder& read(P9Protocol::Stat* stat) {
        return read(&stat->size)
                .read(&stat->type)
                .read(&stat->dev)
                .read(&(stat->qid))
                .read(&stat->mode)
                .read(&stat->atime)
                .read(&stat->mtime)
                .read(&stat->length)
                .read(&(stat->name))
                .read(&(stat->uid))
                .read(&(stat->gid))
                .read(&(stat->muid));
    }

    P9Decoder& read(ImmutableMemoryView* data) {

        P9Protocol::size_type dataSize = 0;
        read(&dataSize);
        *data = _src.viewRemaining().slice(0, dataSize);
        _src.advance(dataSize);

        return (*this);
    }

    P9Decoder& read(Path* path) {
        uint16 componentsCount = 0;
        read(&componentsCount);

        // FIXME: This is where PathBuilder will be handy.
        Path newPath;
        for (uint16 i = 0; i < componentsCount; ++i) {
            String component;
            read(&component);

            // FIXME: Performance kick in the nuts!
            newPath = Path::join(newPath, component);
        }

        *path = std::move(newPath);
        return (*this);
    }

private:
    ByteBuffer& _src;
};



P9Protocol::Tag P9Protocol::createVersionRequest(Tag, ByteBuffer& dest, const String& version) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(maxPossibleMessageSize()) +                // Negotiated message size field
            protocolSize(version);   // Version string data

    P9Encoder(dest)
            .header(MessageType::TVersion, NO_TAG, payloadSize)
            .encode(maxPossibleMessageSize())
            .encode(version);

    return NO_TAG;
}


P9Protocol::Tag
P9Protocol::createAuthRequest(Tag tag, ByteBuffer& dest, fid_type afid,
                              const String& userName, const String& attachName) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(afid) +                  // Proposed fid for authentication mechanism
            protocolSize(userName) +       // User name
            protocolSize(attachName);     // Root name where we want to attach to


    P9Encoder(dest)
            .header(MessageType::TAuth, tag, payloadSize)
            .encode(afid)
            .encode(userName)
            .encode(attachName);

    return tag;
}

P9Protocol::Tag
P9Protocol::createAttachRequest(Tag tag, ByteBuffer& dest,
                                      fid_type fid, fid_type afid,
                                      const String& userName, const String& attachName) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +                  // Proposed fid for the attached root
            protocolSize(afid) +                  // Fid of the passed authentication
            protocolSize(userName) +      // User name
            protocolSize(attachName);     // Root name where we want to attach to

    P9Encoder(dest)
            .header(MessageType::TAttach, tag, payloadSize)
            .encode(fid)
            .encode(afid)
            .encode(userName)
            .encode(attachName);

    return tag;
}

P9Protocol::Tag
P9Protocol::createClunkRequest(Tag tag, Solace::ByteBuffer& dest, fid_type fid) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid);               // Fid to forget

    P9Encoder(dest)
            .header(MessageType::TClunk, tag, payloadSize)
            .encode(fid);

    return tag;
}


P9Protocol::Tag
P9Protocol::createFlushRequest(Tag tag, Solace::ByteBuffer& dest, Tag oldTransation) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(oldTransation);               // Fid to forget

    P9Encoder(dest)
            .header(MessageType::TFlush, tag, payloadSize)
            .encode(oldTransation);

    return tag;
}


P9Protocol::Tag
P9Protocol::createRemoveRequest(Tag tag, Solace::ByteBuffer& dest, fid_type fid) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid);               // Fid to forget

    P9Encoder(dest)
            .header(MessageType::TRemove, tag, payloadSize)
            .encode(fid);

    return tag;
}


P9Protocol::Tag
P9Protocol::createOpenRequest(Tag tag, ByteBuffer& dest, fid_type fid, byte mode) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +                // Fid of the file to open
            protocolSize(mode);                // Mode of the file to open

    P9Encoder(dest)
            .header(MessageType::TOpen, tag, payloadSize)
            .encode(fid)
            .encode(mode);

    return tag;
}


P9Protocol::Tag
P9Protocol::createCreateRequest(Tag tag, ByteBuffer& dest,
                                       fid_type fid,
                                       const String& name,
                                       uint32 permissions,
                                       byte mode) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +
            protocolSize(name) +
            protocolSize(permissions) +
            protocolSize(mode);

    P9Encoder(dest)
            .header(MessageType::TCreate, tag, payloadSize)
            .encode(fid)
            .encode(name)
            .encode(permissions)
            .encode(mode);

    return tag;
}


P9Protocol::Tag
P9Protocol::createReadRequest(Tag tag, ByteBuffer& dest,
                              fid_type fid, uint64 offset, size_type count) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +
            protocolSize(offset) +
            protocolSize(count);

    P9Encoder(dest)
            .header(MessageType::TRead, tag, payloadSize)
            .encode(fid)
            .encode(offset)
            .encode(count);

    return tag;
}


P9Protocol::Tag
P9Protocol::createWriteRequest(Tag tag, ByteBuffer& dest,
                               fid_type fid, uint64 offset, const ImmutableMemoryView& data) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +
            protocolSize(offset) +
            protocolSize(data);

    P9Encoder(dest)
            .header(MessageType::TWrite, tag, payloadSize)
            .encode(fid)
            .encode(offset)
            .encode(data);

    return tag;
}


P9Protocol::Tag
P9Protocol::createWalkRequest(Tag tag, ByteBuffer& dest,
                              fid_type fid, fid_type nfid, const Path& path) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +
            protocolSize(nfid) +
            protocolSize(path);

    P9Encoder(dest)
            .header(MessageType::TWalk, tag, payloadSize)
            .encode(fid)
            .encode(nfid)
            .encode(path);

    return tag;
}


P9Protocol::Tag
P9Protocol::createStatRequest(Tag tag, Solace::ByteBuffer& dest,
                              fid_type fid) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid);

    P9Encoder(dest)
            .header(MessageType::TStat, tag, payloadSize)
            .encode(fid);

    return tag;
}


P9Protocol::Tag
P9Protocol::createWriteStatRequest(Tag tag, Solace::ByteBuffer& dest,
                                   fid_type fid, const Stat& stat) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +
            protocolSize(stat);

    P9Encoder(dest)
            .header(MessageType::TWStat, tag, payloadSize)
            .encode(fid)
            .encode(stat);

    return tag;
}


P9Protocol::Tag
P9Protocol::createSessionRequest(Tag tag, ByteBuffer& dest, const ImmutableMemoryView& key) {
    // Compute message size first:
    const size_type payloadSize =
            8;  // Key size is fixed to be 8 bites.
//            protocolSize(key);

    P9Encoder(dest)
            .header(MessageType::TSession, tag, payloadSize);
//            .encode(key);
    dest.write(key, 8);

    return tag;
}

P9Protocol::Tag
P9Protocol::createShortReadRequest(Tag tag, ByteBuffer& dest,
                                   fid_type fid, const Path& path) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +
            protocolSize(path);

    P9Encoder(dest)
            .header(MessageType::TSRead, tag, payloadSize)
            .encode(fid)
            .encode(path);

    return tag;
}

P9Protocol::Tag
P9Protocol::createShortWriteRequest(Tag tag, ByteBuffer& dest,
                                    fid_type fid, const Path& path, const ImmutableMemoryView& data) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +
            protocolSize(path) +
            protocolSize(data);

    P9Encoder(dest)
            .header(MessageType::TSWrite, tag, payloadSize)
            .encode(fid)
            .encode(path)
            .encode(data);

    return tag;
}


Result<P9Protocol::Response, Error>
P9Protocol::parseNoDataResponse(const MessageHeader& header, ByteBuffer& SOLACE_UNUSED(data)) const {
    Response fcall(header.type, header.tag);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseErrorResponse(const MessageHeader& SOLACE_UNUSED(header), ByteBuffer& data) const {
    String errorMessage;

    P9Decoder(data)
            .read(&errorMessage);

    return Err(Error(errorMessage.c_str()));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseVersionResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.version.msize)
            .read(&fcall.version.version);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseAuthResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.auth.qid);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseAttachResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.attach.qid);

    return Ok(std::move(fcall));
}



Result<P9Protocol::Response, Error>
P9Protocol::parseOpenResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.open.qid)
            .read(&fcall.open.iounit);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseCreateResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.create.qid)
            .read(&fcall.open.iounit);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseReadResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.read.data);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseWriteResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.write.count);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseStatResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.stat);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
P9Protocol::parseWalkResponse(const MessageHeader& header, ByteBuffer& data) const {
    Response fcall(header.type, header.tag);

    P9Decoder decoder(data);

    // FIXME: Non-sense!
    decoder.read(&fcall.walk.nqids);
    for (decltype(fcall.walk.nqids) i = 0; i < fcall.walk.nqids; ++i) {
        decoder.read(&fcall.walk.qids[i]);
    }

    return Ok(std::move(fcall));
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Request parser
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Result<P9Protocol::Request, Error>
parseVersionRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.version.msize)
            .read(&fcall.version.version);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Request, Error>
parseAuthRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.auth.afid)
            .read(&fcall.auth.uname)
            .read(&fcall.auth.aname);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Request, Error>
parseFlushRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.flush.oldtag);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Request, Error>
parseAttachRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.attach.fid)
            .read(&fcall.attach.afid)
            .read(&fcall.attach.uname)
            .read(&fcall.attach.aname);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Request, Error>
parseWalkRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.walk.fid)
            .read(&fcall.walk.path);

    return Ok(std::move(fcall));
}



Result<P9Protocol::Request, Error>
parseOpenRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.open.fid)
            .read(&fcall.open.mode);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Request, Error>
parseCreateRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.create.fid)
            .read(&fcall.create.name)
            .read(&fcall.create.perm)
            .read(&fcall.create.mode);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Request, Error>
parseReadRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.read.fid)
            .read(&fcall.read.offset)
            .read(&fcall.read.count);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Request, Error>
parseWriteRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.write.fid)
            .read(&fcall.write.offset)
            .read(&fcall.write.data);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Request, Error>
parseClunkRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.clunk.fid);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Request, Error>
parseRemoveRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.remove.fid);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Request, Error>
parseStatRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.stat.fid);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Request, Error>
parseWStatRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.wstat.fid)
            .read(&fcall.wstat.stat);

    return Ok(std::move(fcall));
}



Result<P9Protocol::Request, Error>
parseSessionRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

//    P9Decoder(data);
//            .read(&fcall.session.key);

    fcall.session.key = data.viewRemaining().slice(0, 8);
    data.advance(8);

    return Ok(std::move(fcall));
}

Result<P9Protocol::Request, Error>
parseShortReadRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.shortRead.fid)
            .read(&fcall.shortRead.path);

    return Ok(std::move(fcall));
}

Result<P9Protocol::Request, Error>
parseShortWriteRequest(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Request fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.shortWrite.fid)
            .read(&fcall.shortWrite.path)
            .read(&fcall.shortWrite.data);

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
    case MessageType::RWalk:    return parseWalkResponse(header,    data);
    case MessageType::ROpen:    return parseOpenResponse(header,    data);
    case MessageType::RCreate:  return parseCreateResponse(header,  data);
    case MessageType::RSRead:  // Note: RRead is re-used here for RSRead
    case MessageType::RRead:    return parseReadResponse(header,    data);
    case MessageType::RSWrite:  // Note: RWrite is re-used here for RSWrite
    case MessageType::RWrite:   return parseWriteResponse(header,   data);
    case MessageType::RStat:    return parseStatResponse(header,    data);

    // Responses with no data use common procedure:
    case MessageType::RFlush:
    case MessageType::RClunk:
    case MessageType::RRemove:
    case MessageType::RWStat:
    case MessageType::RSession:
        return parseNoDataResponse(header,   data);

    default:
        return Err(Error("Failed to parse responce message: Unsupported message type"));
    }
}

Result<P9Protocol::Request, Solace::Error>
P9Protocol::parseRequest(const MessageHeader& header, ByteBuffer& data) const {
    const auto expectedData = header.size - headerSize();

    // Message data sanity check
    // Make sure we have been given enough data to read a message as requested in the message size.
    if (expectedData > data.remaining())
        return Err(Error("Ill-formed message: Declared frame size larger than message data received"));

    // Make sure there is no extra data in the buffer.
    if (expectedData < data.remaining())
        return Err(Error("Ill-formed message: Declared frame size less than message data received"));

    switch (header.type) {
    case MessageType::TVersion: return parseVersionRequest(header, data);
    case MessageType::TAuth:    return parseAuthRequest(header,    data);
    case MessageType::TFlush:   return parseFlushRequest(header,    data);
    case MessageType::TAttach:  return parseAttachRequest(header,  data);
    case MessageType::TWalk:    return parseWalkRequest(header,    data);
    case MessageType::TOpen:    return parseOpenRequest(header,    data);
    case MessageType::TCreate:  return parseCreateRequest(header,  data);
    case MessageType::TRead:    return parseReadRequest(header,    data);
    case MessageType::TWrite:   return parseWriteRequest(header,   data);
    case MessageType::TClunk:   return parseClunkRequest(header,    data);
    case MessageType::TRemove:  return parseRemoveRequest(header,    data);
    case MessageType::TStat:    return parseStatRequest(header,    data);
    case MessageType::TWStat:   return parseWStatRequest(header,    data);

    case MessageType::TSession: return parseSessionRequest(header,    data);
    case MessageType::TSRead:   return parseShortReadRequest(header,    data);
    case MessageType::TSWrite:  return parseShortWriteRequest(header,    data);

    default:
        return Err(Error("Failed to parse responce message: Unsupported message type"));
    }
}

P9Protocol::size_type P9Protocol::maxNegotiatedMessageSize(size_type newMessageSize) {
    Solace::assertIndexInRange(newMessageSize, 0, maxPossibleMessageSize());
    _maxNegotiatedMessageSize = std::min(newMessageSize, maxPossibleMessageSize());

    return _maxNegotiatedMessageSize;
}


P9Protocol::P9Protocol(size_type maxMassageSize, const Solace::String& version) :
    _maxMassageSize(maxMassageSize),
    _maxNegotiatedMessageSize(maxMassageSize),
    _initialVersion(version),
    _negotiatedVersion(version)
{
    // No-op
}




P9Protocol::Request::Request(MessageType msgType, Tag msgTag) :
    type(msgType),
    tag(msgTag)
{
    switch (type) {
    case MessageType::TVersion: new (&version)  Version;        return;
    case MessageType::TAuth:    new (&auth)     Auth;           return;
    case MessageType::TFlush:   new (&flush)    Flush;          return;
    case MessageType::TAttach:  new (&attach)   Attach;         return;
    case MessageType::TWalk:    new (&walk)     Walk;           return;
    case MessageType::TOpen:    new (&open)     Open;           return;
    case MessageType::TCreate:  new (&create)   Create;         return;
    case MessageType::TRead:    new (&read)     Read;           return;
    case MessageType::TWrite:   new (&write)    Write;          return;
    case MessageType::TClunk:   new (&clunk)    Clunk;          return;
    case MessageType::TRemove:  new (&remove)   Remove;         return;
    case MessageType::TStat:    new (&stat)     StatRequest;    return;
    case MessageType::TWStat:   new (&wstat)    WStat;          return;

    /* 9P2000.e extention */
    case MessageType::TSession: new (&session)      Session;    return;
    case MessageType::TSRead:   new (&shortRead)    SRead;      return;
    case MessageType::TSWrite:  new (&shortWrite)   SWrite;     return;

    default:
        Solace::raise<IOException>("Unexpected message type");
        break;
    }
}

P9Protocol::Request::Request(Request&& rhs) :
    type(std::move(rhs.type)),
    tag(std::move(rhs.tag))
{
    switch (type) {
    case MessageType::TVersion: new (&version)  Version(std::move(rhs.version));    return;
    case MessageType::TAuth:    new (&auth)     Auth(std::move(rhs.auth));       return;
    case MessageType::TFlush:   new (&flush)    Flush(std::move(rhs.flush));      return;
    case MessageType::TAttach:  new (&attach)   Attach(std::move(rhs.attach));     return;
    case MessageType::TWalk:    new (&walk)     Walk(std::move(rhs.walk));       return;
    case MessageType::TOpen:    new (&open)     Open(std::move(rhs.open));       return;
    case MessageType::TCreate:  new (&create)   Create(std::move(rhs.create));     return;
    case MessageType::TRead:    new (&read)     Read(std::move(rhs.read));       return;
    case MessageType::TWrite:   new (&write)    Write(std::move(rhs.write));      return;
    case MessageType::TClunk:   new (&clunk)    Clunk(std::move(rhs.clunk));      return;
    case MessageType::TRemove:  new (&remove)   Remove(std::move(rhs.remove));     return;
    case MessageType::TStat:    new (&stat)     StatRequest(std::move(rhs.stat));       return;
    case MessageType::TWStat:   new (&wstat)    WStat(std::move(rhs.wstat));      return;

    /* 9P2000.e extention */
    case MessageType::TSession: new (&session)      Session(std::move(rhs.session));   return;
    case MessageType::TSRead:   new (&shortRead)    SRead(std::move(rhs.shortRead));  return;
    case MessageType::TSWrite:  new (&shortWrite)   SWrite(std::move(rhs.shortWrite)); return;

    default:
        Solace::raise<IOException>("Unexpected message type");
        break;
    }
}

P9Protocol::Request::~Request() {
    switch (type) {
    case MessageType::TVersion: (&version)->~Version();     break;
    case MessageType::TAuth:    (&auth)->~Auth();           break;
    case MessageType::TFlush:   (&flush)->~Flush();         break;
    case MessageType::TAttach:  (&attach)->~Attach();       break;
    case MessageType::TWalk:    (&walk)->~Walk();           break;
    case MessageType::TOpen:    (&open)->~Open();           break;
    case MessageType::TCreate:  (&create)->~Create();       break;
    case MessageType::TRead:    (&read)->~Read();           break;
    case MessageType::TWrite:   (&write)->~Write();         break;
    case MessageType::TClunk:   (&clunk)->~Clunk();         break;
    case MessageType::TRemove:  (&remove)->~Remove();       break;
    case MessageType::TStat:    (&stat)->~StatRequest();    break;
    case MessageType::TWStat:   (&wstat)->~WStat();         break;

    /* 9P2000.e extention */
    case MessageType::TSession: (&session)->~Session();     break;
    case MessageType::TSRead:   (&shortRead)->~SRead();     break;
    case MessageType::TSWrite:  (&shortWrite)->~SWrite();   break;

    default:
        Solace::raise<IOException>("Unexpected message type");
        break;
    }
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
    case MessageType::RSRead:  // Note: RRead is re-used here for RSRead
    case MessageType::RRead:    new (&read) Read;       return;
    case MessageType::RSWrite:  // Note: RWrite is re-used here for RSWrite
    case MessageType::RWrite:   new (&write) Write;     return;
    case MessageType::RStat:    new (&stat) Stat;       return;
    case MessageType::RClunk:
    case MessageType::RRemove:
    case MessageType::RFlush:
    case MessageType::RWStat:
    case MessageType::RSession:
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
    case MessageType::RSRead:  // Note: RRead is re-used here for RSRead
    case MessageType::RRead:    new (&read) Read(std::move(rhs.read)); return;
    case MessageType::RSWrite:  // Note: RWrite is re-used here for RSWrite
    case MessageType::RWrite:   new (&write) Write(std::move(rhs.write)); return;
    case MessageType::RStat:    new (&stat) Stat(std::move(rhs.stat)); return;
    case MessageType::RClunk:
    case MessageType::RRemove:
    case MessageType::RFlush:
    case MessageType::RWStat:
    case MessageType::RSession:
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
    case MessageType::RSRead:  // Note: RRead is re-used here for RSRead
    case MessageType::RRead:    (&read)->~Read();       break;
    case MessageType::RSWrite:  // Note: RWrite is re-used here for RSWrite
    case MessageType::RWrite:   (&write)->~Write();     break;
    case MessageType::RStat:    (&stat)->~Stat();       break;
    case MessageType::RClunk:
    case MessageType::RRemove:
    case MessageType::RFlush:
    case MessageType::RWStat:
    case MessageType::RSession:
    default:
        break;
    }
}

P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::version(const String& version, size_type maxMessageSize) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(maxMessageSize) +                // Negotiated message size field
            protocolSize(version);   // Version string data

    P9Encoder(buffer())
            .header(MessageType::RVersion, NO_TAG, payloadSize)
            .encode(maxMessageSize)
            .encode(version);

    return (*this);
}

P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::auth(Tag tag, const Qid& qid) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(qid);   // Version string data

    P9Encoder(buffer())
            .header(MessageType::RAuth, tag, payloadSize)
            .encode(qid);

    return (*this);
}

P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::error(Tag tag, const String& message) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(message);   // Version string data

    P9Encoder(buffer())
            .header(MessageType::RError, tag, payloadSize)
            .encode(message);

    return (*this);
}

P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::flush(Tag tag) {
    P9Encoder(buffer())
            .header(MessageType::RFlush, tag);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::attach(Tag tag, const Qid& qid) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(qid);   // Version string data

    P9Encoder(buffer())
            .header(MessageType::RAttach, tag, payloadSize)
            .encode(qid);

    return (*this);
}

P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::walk(Tag tag, const Array<Qid>& qids) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(qids);   // Version string data

    P9Encoder(buffer())
            .header(MessageType::RWalk, tag, payloadSize)
            .encode(qids);

    return (*this);
}

P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::open(Tag tag, const Qid& qid, size_type iounit) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(qid) +
            protocolSize(iounit);

    P9Encoder(buffer())
            .header(MessageType::ROpen, tag, payloadSize)
            .encode(qid)
            .encode(iounit);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::create(Tag tag, const Qid& qid, size_type iounit) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(qid) +
            protocolSize(iounit);

    P9Encoder(buffer())
            .header(MessageType::RCreate, tag, payloadSize)
            .encode(qid)
            .encode(iounit);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::read(Tag tag, const ImmutableMemoryView& data) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(data);

    P9Encoder(buffer())
            .header(MessageType::RRead, tag, payloadSize)
            .encode(data);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::write(Tag tag, size_type count) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(count);

    P9Encoder(buffer())
            .header(MessageType::RWrite, tag, payloadSize)
            .encode(count);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::clunk(Tag tag) {
    P9Encoder(buffer())
            .header(MessageType::RClunk, tag);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::remove(Tag tag) {
    P9Encoder(buffer())
            .header(MessageType::RRemove, tag);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::stat(Tag tag, const Stat& data) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(data);

    P9Encoder(buffer())
            .header(MessageType::RStat, tag, payloadSize)
            .encode(data);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::wstat(Tag tag) {
    P9Encoder(buffer())
            .header(MessageType::RWStat, tag);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::session(Tag tag) {
    P9Encoder(buffer())
            .header(MessageType::RSession, tag);

    return (*this);
}



P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::shortRead(Tag tag, const ImmutableMemoryView& data) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(data);

    P9Encoder(buffer())
            .header(MessageType::RSRead, tag, payloadSize)
            .encode(data);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::shortWrite(Tag tag, size_type count) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(count);

    P9Encoder(buffer())
            .header(MessageType::RSWrite, tag, payloadSize)
            .encode(count);

    return (*this);
}
