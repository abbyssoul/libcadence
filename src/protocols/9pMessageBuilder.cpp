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


    P9Encoder& encode(uint8 value) {
        _dest.writeLE(value);
        return (*this);
    }

    P9Encoder& encode(uint16 value) {
        _dest.writeLE(value);
        return (*this);
    }

    P9Encoder& encode(uint32 value) {
        _dest.writeLE(value);
        return (*this);
    }

    P9Encoder& encode(uint64 value) {
        _dest.writeLE(value);
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



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Request Builder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::version(const String& version, size_type maxMessageSize) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(maxMessageSize) +                // Negotiated message size field
            protocolSize(version);   // Version string data

    P9Encoder(buffer())
            .header(MessageType::TVersion, NO_TAG, payloadSize)
            .encode(maxMessageSize)
            .encode(version);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::auth(fid_type afid, const String& userName, const String& attachName) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(afid) +                  // Proposed fid for authentication mechanism
            protocolSize(userName) +       // User name
            protocolSize(attachName);     // Root name where we want to attach to


    P9Encoder(buffer())
            .header(MessageType::TAuth, _tag, payloadSize)
            .encode(afid)
            .encode(userName)
            .encode(attachName);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::attach(fid_type fid, fid_type afid, const String& userName, const String& attachName) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +                  // Proposed fid for the attached root
            protocolSize(afid) +                  // Fid of the passed authentication
            protocolSize(userName) +      // User name
            protocolSize(attachName);     // Root name where we want to attach to

    P9Encoder(buffer())
            .header(MessageType::TAttach, _tag, payloadSize)
            .encode(fid)
            .encode(afid)
            .encode(userName)
            .encode(attachName);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::clunk(fid_type fid) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid);               // Fid to forget

    P9Encoder(buffer())
            .header(MessageType::TClunk, _tag, payloadSize)
            .encode(fid);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::flush(Tag oldTransation) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(oldTransation);               // Transaction to forget

    P9Encoder(buffer())
            .header(MessageType::TFlush, _tag, payloadSize)
            .encode(oldTransation);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::remove(fid_type fid) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid);               // Fid to remove

    P9Encoder(buffer())
            .header(MessageType::TRemove, _tag, payloadSize)
            .encode(fid);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::open(fid_type fid, byte mode) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +                // Fid of the file to open
            protocolSize(mode);                // Mode of the file to open

    P9Encoder(buffer())
            .header(MessageType::TOpen, _tag, payloadSize)
            .encode(fid)
            .encode(mode);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::create(fid_type fid, const String& name, uint32 permissions, byte mode) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +
            protocolSize(name) +
            protocolSize(permissions) +
            protocolSize(mode);

    P9Encoder(buffer())
            .header(MessageType::TCreate, _tag, payloadSize)
            .encode(fid)
            .encode(name)
            .encode(permissions)
            .encode(mode);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::read(fid_type fid, uint64 offset, size_type count) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +
            protocolSize(offset) +
            protocolSize(count);

    P9Encoder(buffer())
            .header(MessageType::TRead, _tag, payloadSize)
            .encode(fid)
            .encode(offset)
            .encode(count);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::write(fid_type fid, uint64 offset, const ImmutableMemoryView& data) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +
            protocolSize(offset) +
            protocolSize(data);

    P9Encoder(buffer())
            .header(MessageType::TWrite, _tag, payloadSize)
            .encode(fid)
            .encode(offset)
            .encode(data);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::walk(fid_type fid, fid_type nfid, const Path& path) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +
            protocolSize(nfid) +
            protocolSize(path);

    P9Encoder(buffer())
            .header(MessageType::TWalk, _tag, payloadSize)
            .encode(fid)
            .encode(nfid)
            .encode(path);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::stat(fid_type fid) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid);

    P9Encoder(buffer())
            .header(MessageType::TStat, _tag, payloadSize)
            .encode(fid);

    return (*this);
}


P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::writeStat(fid_type fid, const Stat& stat) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(fid) +
            protocolSize(stat);

    P9Encoder(buffer())
            .header(MessageType::TWStat, _tag, payloadSize)
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

    P9Encoder(buffer())
            .header(MessageType::TSession, _tag, payloadSize);
//            .encode(key);
    buffer().write(key, 8);

    return (*this);
}

P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::shortRead(fid_type rootFid, const Path& path) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(rootFid) +
            protocolSize(path);

    P9Encoder(buffer())
            .header(MessageType::TSRead, _tag, payloadSize)
            .encode(rootFid)
            .encode(path);

    return (*this);
}

P9Protocol::RequestBuilder&
P9Protocol::RequestBuilder::shortWrite(fid_type rootFid, const Path& path, const ImmutableMemoryView& data) {
    // Compute message size first:
    const size_type payloadSize =
            protocolSize(rootFid) +
            protocolSize(path) +
            protocolSize(data);

    P9Encoder(buffer())
            .header(MessageType::TSWrite, _tag, payloadSize)
            .encode(rootFid)
            .encode(path)
            .encode(data);

    return (*this);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Response Builder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
