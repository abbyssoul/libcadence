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



P9Protocol::size_type
P9Protocol::Encoder::protocolSize(const uint8& value) {
    return sizeof(value);
}

P9Protocol::size_type
P9Protocol::Encoder::protocolSize(const uint16& value) {
    return sizeof(value);
}

P9Protocol::size_type
P9Protocol::Encoder::protocolSize(const uint32& value) {
    return sizeof(value);
}

P9Protocol::size_type
P9Protocol::Encoder::protocolSize(const uint64& value) {
    return sizeof(value);
}

P9Protocol::size_type
P9Protocol::Encoder::protocolSize(const String& str) {
    return sizeof(uint16) +         // Space for string var size
            str.size();             // Space for the actual string bytes
}


P9Protocol::size_type
P9Protocol::Encoder::protocolSize(const Path& path) {
    P9Protocol::size_type payloadSize = 0;
    for (auto& segment : path) {
        payloadSize += P9Protocol::Encoder::protocolSize(segment);
    }

    return sizeof(uint16) +  // Var number of segments
            payloadSize;
}


P9Protocol::size_type
P9Protocol::Encoder::protocolSize(const Qid&) {
    static constexpr size_type kQidSize = sizeof(Qid::type) +
            sizeof(Qid::version) +
            sizeof(Qid::path);

    static_assert(kQidSize == 13, "Incorrect Qid struct size");

    return kQidSize;
}


P9Protocol::size_type
P9Protocol::Encoder::protocolSize(const Stat& stat) {
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


P9Protocol::size_type
P9Protocol::Encoder::protocolSize(const Array<Qid>& qids) {
    Qid uselessQid;

    return sizeof(uint16) +  // Var number of elements
            qids.size() * protocolSize(uselessQid);
}


P9Protocol::size_type
P9Protocol::Encoder::protocolSize(const ImmutableMemoryView& data) {
    return sizeof(size_type) +  // Var number of elements
            data.size();
}


/**
 * P9 protocol helper class to encode data into a protocol message format.
 */
P9Protocol::Encoder&
P9Protocol::Encoder::header(P9Protocol::MessageType type, P9Protocol::Tag tag, P9Protocol::size_type payloadSize) {
    return encode(P9Protocol::headerSize() + payloadSize)
            .encode(static_cast<byte>(type))
            .encode(tag);
}


P9Protocol::Encoder&
P9Protocol::Encoder::encode(uint8 value) {
    _dest.writeLE(value);
    return (*this);
}

P9Protocol::Encoder&
P9Protocol::Encoder::encode(uint16 value) {
    _dest.writeLE(value);
    return (*this);
}

P9Protocol::Encoder&
P9Protocol::Encoder::encode(uint32 value) {
    _dest.writeLE(value);
    return (*this);
}

P9Protocol::Encoder&
P9Protocol::Encoder::encode(uint64 value) {
    _dest.writeLE(value);
    return (*this);
}

P9Protocol::Encoder&
P9Protocol::Encoder::encode(const uint16 dataSize, const char* str) {
    encode(dataSize);
    _dest.write(str, dataSize);

    return (*this);
}

P9Protocol::Encoder&
P9Protocol::Encoder::encode(const String& str) {
    return encode(str.size(), str.c_str());
}

P9Protocol::Encoder&
P9Protocol::Encoder::encode(const P9Protocol::Qid& qid) {
    return encode(qid.type)
            .encode(qid.version)
            .encode(qid.path);
}

P9Protocol::Encoder&
P9Protocol::Encoder::encode(const Array<P9Protocol::Qid>& qids) {
    encode(static_cast<uint16>(qids.size()));
    for (auto qid : qids) {
        encode(qid);
    }

    return (*this);
}

P9Protocol::Encoder&
P9Protocol::Encoder::encode(const P9Protocol::Stat& stat) {
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

P9Protocol::Encoder&
P9Protocol::Encoder::encode(const ImmutableMemoryView& data) {
    encode(static_cast<P9Protocol::size_type>(data.size()));
    _dest.write(data);

    return (*this);
}

P9Protocol::Encoder&
P9Protocol::Encoder::encode(const Path& path) {
    encode(static_cast<uint16>(path.getComponentsCount()));

    for (const auto& component : path) {
        encode(component);
    }

    return (*this);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Request Builder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ByteBuffer&
P9Protocol::RequestBuilder::build() {
    if (type() < MessageType::_beginSupportedMessageCode ||
        type() > MessageType::_endSupportedMessageCode) {
        Solace::raise<IOException>("Unexpected message type");
    }

    return _buffer.flip();
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::version(const String& version, size_type maxMessageSize) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(maxMessageSize) +                // Negotiated message size field
            encode.protocolSize(version);   // Version string data

    // Note: we ignore what ever tag value been passed and use NO_TAG value as per protocol.
    _tag = NO_TAG;
    _type = MessageType::TVersion;
    encode.header(type(), NO_TAG, payloadSize)
            .encode(maxMessageSize)
            .encode(version);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::auth(Fid afid, const String& userName, const String& attachName) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(afid) +                  // Proposed fid for authentication mechanism
            encode.protocolSize(userName) +       // User name
            encode.protocolSize(attachName);     // Root name where we want to attach to

    _type = MessageType::TAuth;
    encode.header(type(), _tag, payloadSize)
            .encode(afid)
            .encode(userName)
            .encode(attachName);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::attach(Fid fid, Fid afid, const String& userName, const String& attachName) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(fid) +                  // Proposed fid for the attached root
            encode.protocolSize(afid) +                  // Fid of the passed authentication
            encode.protocolSize(userName) +      // User name
            encode.protocolSize(attachName);     // Root name where we want to attach to

    _type = MessageType::TAttach;
    encode.header(type(), _tag, payloadSize)
            .encode(fid)
            .encode(afid)
            .encode(userName)
            .encode(attachName);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::clunk(Fid fid) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(fid);               // Fid to forget

    _type = MessageType::TClunk;
    encode.header(type(), _tag, payloadSize)
            .encode(fid);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::flush(Tag oldTransation) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(oldTransation);               // Transaction to forget

    _type = MessageType::TFlush;
    encode.header(type(), _tag, payloadSize)
            .encode(oldTransation);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::remove(Fid fid) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(fid);               // Fid to remove

    _type = MessageType::TRemove;
    encode.header(type(), _tag, payloadSize)
            .encode(fid);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::open(Fid fid, P9Protocol::OpenMode mode) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(fid) +                // Fid of the file to open
            encode.protocolSize(static_cast<byte>(mode));                // Mode of the file to open

    _type = MessageType::TOpen;
    encode.header(type(), _tag, payloadSize)
            .encode(fid)
            .encode(static_cast<byte>(mode));

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::create(Fid fid, const String& name, uint32 permissions, P9Protocol::OpenMode mode) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(fid) +
            encode.protocolSize(name) +
            encode.protocolSize(permissions) +
            encode.protocolSize(static_cast<byte>(mode));

    _type = MessageType::TCreate;
    encode.header(type(), _tag, payloadSize)
            .encode(fid)
            .encode(name)
            .encode(permissions)
            .encode(static_cast<byte>(mode));

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::read(Fid fid, uint64 offset, size_type count) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(fid) +
            encode.protocolSize(offset) +
            encode.protocolSize(count);

    _type = MessageType::TRead;
    encode.header(type(), _tag, payloadSize)
            .encode(fid)
            .encode(offset)
            .encode(count);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::write(Fid fid, uint64 offset, const ImmutableMemoryView& data) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(fid) +
            encode.protocolSize(offset) +
            encode.protocolSize(data);

    _type = MessageType::TWrite;
    encode.header(type(), _tag, payloadSize)
            .encode(fid)
            .encode(offset)
            .encode(data);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::walk(Fid fid, Fid nfid, const Path& path) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(fid) +
            encode.protocolSize(nfid) +
            encode.protocolSize(path);

    _type = MessageType::TWalk;
    encode.header(type(), _tag, payloadSize)
            .encode(fid)
            .encode(nfid)
            .encode(path);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::stat(Fid fid) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(fid);

    _type = MessageType::TStat;
    encode.header(type(), _tag, payloadSize)
            .encode(fid);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::writeStat(Fid fid, const Stat& stat) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(fid) +
            encode.protocolSize(stat);

    _type = MessageType::TWStat;
    encode.header(type(), _tag, payloadSize)
            .encode(fid)
            .encode(stat);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::session(const ImmutableMemoryView& key) {
    // Compute message size first:
    const size_type payloadSize =
            8;  // Key size is fixed to be 8 bites.
//            protocolSize(key);

    _type = MessageType::TSession;
    Encoder(buffer())
            .header(type(), _tag, payloadSize);
//            .encode(key);
    buffer().write(key, 8);

    return (*this);
}

P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::shortRead(Fid rootFid, const Path& path) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(rootFid) +
            encode.protocolSize(path);

    _type = MessageType::TSRead;
    encode.header(type(), _tag, payloadSize)
            .encode(rootFid)
            .encode(path);

    return (*this);
}

P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::shortWrite(Fid rootFid, const Path& path, const ImmutableMemoryView& data) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(rootFid) +
            encode.protocolSize(path) +
            encode.protocolSize(data);

    _type = MessageType::TSWrite;
    encode.header(type(), _tag, payloadSize)
            .encode(rootFid)
            .encode(path)
            .encode(data);

    return (*this);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Response Builder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ByteBuffer&
P9Protocol::ResponseBuilder::build() {
    if (type() < MessageType::_beginSupportedMessageCode ||
        type() > MessageType::_endSupportedMessageCode) {
        Solace::raise<IOException>("Unexpected message type");
    }

    return _buffer.flip();
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::version(const String& version, size_type maxMessageSize) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(maxMessageSize) +                // Negotiated message size field
            encode.protocolSize(version);   // Version string data

    _type = MessageType::RVersion;
    encode.header(type(), NO_TAG, payloadSize)
            .encode(maxMessageSize)
            .encode(version);

    return (*this);
}

P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::auth(const Qid& qid) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(qid);   // Version string data

    _type = MessageType::RAuth;
    encode.header(type(), _tag, payloadSize)
            .encode(qid);

    return (*this);
}

P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::error(const String& message) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(message);   // Version string data

    _type = MessageType::RError;
    encode.header(type(), _tag, payloadSize)
            .encode(message);

    return (*this);
}

P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::flush() {
    _type = MessageType::RFlush;

    Encoder(buffer())
            .header(type(), _tag);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::attach(const Qid& qid) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(qid);   // Version string data

    _type = MessageType::RAttach;
    encode.header(type(), _tag, payloadSize)
            .encode(qid);

    return (*this);
}

P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::walk(const Array<Qid>& qids) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(qids);   // Version string data

    _type = MessageType::RWalk;
    encode.header(type(), _tag, payloadSize)
            .encode(qids);

    return (*this);
}

P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::open(const Qid& qid, size_type iounit) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(qid) +
            encode.protocolSize(iounit);

    _type = MessageType::ROpen;
    encode.header(type(), _tag, payloadSize)
            .encode(qid)
            .encode(iounit);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::create(const Qid& qid, size_type iounit) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(qid) +
            encode.protocolSize(iounit);

    _type = MessageType::RCreate;
    encode.header(type(), _tag, payloadSize)
            .encode(qid)
            .encode(iounit);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::read(const ImmutableMemoryView& data) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(data);

    _type = MessageType::RRead;
    encode.header(type(), _tag, payloadSize)
            .encode(data);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::write(size_type count) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(count);

    _type = MessageType::RWrite;
    encode.header(type(), _tag, payloadSize)
            .encode(count);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::clunk() {
    _type = MessageType::RClunk;

    Encoder(buffer())
            .header(type(), _tag);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::remove() {
    _type = MessageType::RRemove;

    Encoder(buffer())
            .header(type(), _tag);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::stat(const Stat& data) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            sizeof(uint16) +
            encode.protocolSize(data);

    _type = MessageType::RStat;
    encode.header(type(), _tag, payloadSize)
            .encode(static_cast<uint16>(payloadSize))
            .encode(data);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::wstat() {
    _type = MessageType::RWStat;

    Encoder(buffer())
            .header(type(), _tag);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::session() {
    _type = MessageType::RSession;

    Encoder(buffer())
            .header(type(), _tag);

    return (*this);
}



P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::shortRead(const ImmutableMemoryView& data) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(data);

    _type = MessageType::RSRead;
    encode.header(type(), _tag, payloadSize)
            .encode(data);

    return (*this);
}


P9Protocol::ResponseBuilder&
P9Protocol::ResponseBuilder::shortWrite(size_type count) {
    Encoder encode(buffer());

    // Compute message size first:
    const size_type payloadSize =
            encode.protocolSize(count);

    _type = MessageType::RSWrite;
    encode.header(type(), _tag, payloadSize)
            .encode(count);

    return (*this);
}
