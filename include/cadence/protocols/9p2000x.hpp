/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
#pragma once
#ifndef CADENCE_PROTOCOLS_9P2000X_HPP
#define CADENCE_PROTOCOLS_9P2000X_HPP

#include <solace/string.hpp>
#include <solace/byteBuffer.hpp>
#include <solace/version.hpp>
#include <solace/result.hpp>
#include <solace/error.hpp>
#include <solace/path.hpp>


namespace cadence {

/**
 * A protocol a collection of well known methods to create protocol messages.
 * Essantially it is a factory of message formatted bytes.
 * This bytes - or message frame can be written into a network to communicate with other peers that
 * understand this protocol.
 *
 */
class P9Protocol {
public:
    typedef Solace::uint32 size_type;
    typedef Solace::uint16 Tag;
    typedef Solace::uint32 fid_type;

    static const Tag NO_TAG;
    static const fid_type NOFID;
    static const Solace::String PROTOCOL_VERSION;

    enum Consts {
        MAX_WELEM = 16
    };

    /* 9P message types */
    enum class MessageType : Solace::byte {
        TVersion = 100,
        RVersion,
        TAuth = 102,
        RAuth,
        TAttach = 104,
        RAttach,
        TError = 106, /* illegal */
        RError,
        TFlush = 108,
        RFlush,
        TWalk = 110,
        RWalk,
        TOpen = 112,
        ROpen,
        TCreate = 114,
        RCreate,
        TRead = 116,
        RRead,
        TWrite = 118,
        RWrite,
        TClunk = 120,
        RClunk,
        TRemove = 122,
        RRemove,
        TStat = 124,
        RStat,
        TWStat = 126,
        RWStat,
    };

    /**
     * The qid represents the server's unique identification for the file being accessed:
     * two files on the same server hierarchy are the same if and only if their qids are the same.
     */
    struct Qid {
        Solace::byte	type;
        Solace::uint32	version;
        Solace::uint64  path;
    };

    struct Stat {
        Solace::uint16  size;
        Solace::uint16  type;
        Solace::uint32 	dev;
        Qid	qid;
        Solace::uint32	mode;
        Solace::uint32	atime;
        Solace::uint32	mtime;
        Solace::uint64	length;
        Solace::String	name;
        Solace::String  uid;
        Solace::String  gid;
        Solace::String  muid;
    };

    struct MessageHeader {
        size_type       size;
        MessageType     type;
        Tag        tag;
    };

    struct Response {

        struct Write {
            Solace::uint32 count;
        };

        struct Version {
            size_type       msize;
            Solace::String  version;
        };

        struct Error {
            Solace::String  ename;
        };

        struct Auth {
            Qid  qid;
        };

        struct Attach {
            Qid  qid;
        };

        struct Walk {
            Solace::uint16 nqids;
            Qid     qids[MAX_WELEM];
        };

        struct Open {
            Qid  qid;
            Solace::uint32 iounit;
        };

        struct Create {
            Qid  qid;
            Solace::uint32 iounit;
        };

        struct Read {
            Solace::MemoryView data;
        };


        MessageType type;
        Tag         tag;
        union {
            Write write;
            Version version;
            Error error;
            Auth auth;
            Attach attach;
            Walk walk;
            Open open;
            Create create;
            Read read;
            Stat  stat;
        };

        Response(MessageType rtype, Tag tag);
        Response(Response&& rhs);

        ~Response();
    };


public:

    virtual ~P9Protocol() = default;

    P9Protocol();

    //---------------------------------------------------------
    // Create protocol requests
    //---------------------------------------------------------
    Tag createVersionRequest(Tag tag, Solace::ByteBuffer& dest, const Solace::String& version = PROTOCOL_VERSION);

    Tag createAuthRequest(Tag tag, Solace::ByteBuffer& dest,
                               fid_type afid, const Solace::String& userName, const Solace::String& attachName);

    Tag createAttachRequest(Tag tag, Solace::ByteBuffer& dest,
                                 fid_type fid, fid_type afid,
                                 const Solace::String& userName, const Solace::String& attachName);

    Tag createClunkRequest(Tag tag, Solace::ByteBuffer& dest, fid_type fid);
    Tag createFlushRequest(Tag tag, Solace::ByteBuffer& dest, Tag oldTransation);
    Tag createRemoveRequest(Tag tag, Solace::ByteBuffer& dest, fid_type fid);

    Tag createOpenRequest(Tag tag, Solace::ByteBuffer& dest, fid_type fid, Solace::byte mode);
    Tag createCreateRequest(Tag tag, Solace::ByteBuffer& dest,
                                 fid_type fid,
                                 const Solace::String& name,
                                 Solace::uint32 permissions,
                                 Solace::byte mode);

    Tag createReadRequest(Tag tag, Solace::ByteBuffer& dest,
                               fid_type fid, Solace::uint64 offset, size_type count);

    Tag createWriteRequest(Tag tag, Solace::ByteBuffer& dest,
                               fid_type fid, Solace::uint64 offset, size_type count, const Solace::byte* data);

    Tag createWalkRequest(Tag tag, Solace::ByteBuffer& dest,
                               fid_type fid, fid_type nfid, const Solace::Path& path);

    Tag createStatRequest(Tag tag, Solace::ByteBuffer& dest,
                               fid_type fid);

    Tag createWriteStatRequest(Tag tag, Solace::ByteBuffer& dest,
                               fid_type fid, const Stat& stat);


    size_type maxPossibleMessageSize() const noexcept;
    size_type maxNegotiatedMessageSize() const noexcept {
        return _maxNegotiatedMessageSize;
    }

    size_type maxNegotiatedMessageSize(size_type newMessageSize) noexcept {
        _maxNegotiatedMessageSize = std::min(newMessageSize, maxPossibleMessageSize());
        return _maxNegotiatedMessageSize;
    }

    size_type headerSize() const noexcept {
        // Note: can't use sizeof(MessageHeader) due to padding
        return  sizeof(MessageHeader::size) +
                sizeof(MessageHeader::type) +
                sizeof(MessageHeader::tag);
    }

    Solace::Result<MessageHeader, Solace::Error>
    parseMessageHeader(Solace::ByteBuffer& buffer) const;

    Solace::Result<Response, Solace::Error> parseMessage(const MessageHeader& header, Solace::ByteBuffer& data);

//    fid_type allocateFid();
//    Tag nextTag();

protected:

    Solace::Result<Response, Solace::Error> parseErrorResponse(const MessageHeader& header, Solace::ByteBuffer& data);
    Solace::Result<Response, Solace::Error> parseVersionResponse(const MessageHeader& header, Solace::ByteBuffer& data);
    Solace::Result<Response, Solace::Error> parseAuthResponse(const MessageHeader& header, Solace::ByteBuffer& data);
    Solace::Result<Response, Solace::Error> parseAttachResponse(const MessageHeader& header, Solace::ByteBuffer& data);

    Solace::Result<Response, Solace::Error> parseClunkResponse(const MessageHeader& header, Solace::ByteBuffer& data);
    Solace::Result<Response, Solace::Error> parseFlushResponse(const MessageHeader& header, Solace::ByteBuffer& data);
    Solace::Result<Response, Solace::Error> parseOpenResponse(const MessageHeader& header, Solace::ByteBuffer& data);
    Solace::Result<Response, Solace::Error> parseCreateResponse(const MessageHeader& header, Solace::ByteBuffer& data);
    Solace::Result<Response, Solace::Error> parseReadResponse(const MessageHeader& header, Solace::ByteBuffer& data);
    Solace::Result<Response, Solace::Error> parseWriteResponse(const MessageHeader& header, Solace::ByteBuffer& data);
    Solace::Result<Response, Solace::Error> parseRemoveResponse(const MessageHeader& header, Solace::ByteBuffer& data);

    Solace::Result<Response, Solace::Error> parseStatResponse(const MessageHeader& header, Solace::ByteBuffer& data);
    Solace::Result<Response, Solace::Error> parseWStatResponse(const MessageHeader& header, Solace::ByteBuffer& data);
    Solace::Result<Response, Solace::Error> parseWalkResponse(const MessageHeader& header, Solace::ByteBuffer& data);

private:

    size_type   _maxNegotiatedMessageSize;
    Tag    _currentTag;
};

}  // end of namespace cadence
#endif  // CADENCE_PROTOCOLS_9P2000X_HPP
