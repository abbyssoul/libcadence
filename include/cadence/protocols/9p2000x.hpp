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

    /**
     * Maximum frame size that can transmitted by the protocol.
     * (Note: server/client can negotiate actual frame size to be less then that.
     */
    static const size_type MAX_MESSAGE_SIZE;

    /**
     * String representing version of protocol.
     */
    static const Solace::String PROTOCOL_VERSION;

    /**
     * Special value of a message tag representing 'no tag'.
     */
    static const Tag NO_TAG;
    static const fid_type NOFID;

    enum Consts {
        MAX_WELEM = 16
    };

    /* 9P message types */
    enum class MessageType : Solace::byte {
        _beginSupportedMessageCode = 100,
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

        /**
         * 9P2000.e extension
         */
        TSession = 150,
        RSession,
        TSRead = 152,
        RSRead,
        TSWrite = 154,
        RSWrite,

        _endSupportedMessageCode
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
        Qid             qid;
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


    struct Request {
        MessageType type;
        Tag         tag;

        struct Version {
            size_type       msize;
            Solace::String  version;
        };

        struct Auth {
            fid_type        afid;
            Solace::String  uname;
            Solace::String  aname;
        };

        struct Flush {
            Tag        oldtag;
        };

        struct Attach {
            fid_type        fid;
            fid_type        afid;
            Solace::String  uname;
            Solace::String  aname;
        };

        struct Walk {
            fid_type        fid;
            fid_type        newfid;
            Solace::Path    path;
        };

        struct Open {
            fid_type        fid;
            Solace::uint8   mode;
        };

        struct Create {
            fid_type        fid;
            Solace::String  name;
            Solace::uint32  perm;
            Solace::uint8   mode;
        };

        struct Read {
            fid_type        fid;
            Solace::uint64  offset;
            Solace::uint32  count;
        };

        struct Write {
            fid_type        fid;
            Solace::uint64  offset;
            Solace::ImmutableMemoryView data;
        };

        struct Clunk {
            fid_type        fid;
        };

        struct Remove {
            fid_type        fid;
        };

        struct StatRequest {
            fid_type        fid;
        };

        struct WStat {
            fid_type        fid;
            Stat            stat;
        };


        struct Session {
            Solace::ImmutableMemoryView key;
        };

        struct SRead {
            fid_type        fid;
            Solace::Path    path;
        };

        struct SWrite {
            fid_type        fid;
            Solace::Path    path;
            Solace::ImmutableMemoryView data;
        };


        union {
            Version     version;
            Auth        auth;
            Flush       flush;
            Attach      attach;
            Walk        walk;
            Open        open;
            Create      create;
            Read        read;
            Write       write;
            Clunk       clunk;
            Remove      remove;
            StatRequest stat;
            WStat       wstat;

            /* 9P2000.e extention */
            Session     session;
            SRead       shortRead;
            SWrite      shortWrite;
        };

        Request(MessageType rtype, Tag tag);
        Request(Request&& rhs);

        ~Request();
    };

    struct Response {

        struct Write {
            size_type       count;
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
            size_type iounit;
        };

        struct Create {
            Qid  qid;
            size_type iounit;
        };

        struct Read {
            Solace::MemoryView data;
        };


        MessageType type;
        Tag         tag;
        union {
            Write       write;
            Version     version;
            Error       error;
            Auth        auth;
            Attach      attach;
            Walk        walk;
            Open        open;
            Create      create;
            Read        read;
            Stat        stat;
        };

        Response(MessageType rtype, Tag tag);
        Response(Response&& rhs);

        ~Response();
    };

    /**
     * Helper class to build response messages.
     */
    class ResponseBuilder {
    public:

        ResponseBuilder(Solace::ByteBuffer& buffer) :
            _buffer(buffer),
            _sizeMark(buffer.position())
        {}

        Solace::ByteBuffer& buffer() {
            return _buffer;
        }

        Solace::ByteBuffer& build();

        ResponseBuilder& version(const Solace::String& version, size_type maxMessageSize = MAX_MESSAGE_SIZE);
        ResponseBuilder& auth(Tag tag, const Qid& qid);
        ResponseBuilder& error(Tag tag, const Solace::String& message);
        ResponseBuilder& flush(Tag tag);
        ResponseBuilder& attach(Tag tag, const Qid& qid);
        ResponseBuilder& walk(Tag tag, const Solace::Array<Qid>& qids);
        ResponseBuilder& open(Tag tag, const Qid& qid, size_type iounit);
        ResponseBuilder& create(Tag tag, const Qid& qid, size_type iounit);
        ResponseBuilder& read(Tag tag, const Solace::ImmutableMemoryView& data);
        ResponseBuilder& write(Tag tag, size_type iounit);
        ResponseBuilder& clunk(Tag tag);
        ResponseBuilder& remove(Tag tag);
        ResponseBuilder& stat(Tag tag, const Stat& value);
        ResponseBuilder& wstat(Tag tag);

        /* 9P2000.e extention */
        ResponseBuilder& session(Tag tag);
        ResponseBuilder& shortRead(Tag tag, const Solace::ImmutableMemoryView& data);
        ResponseBuilder& shortWrite(Tag tag, size_type iounit);

    private:
        Solace::ByteBuffer&             _buffer;
        Solace::ByteBuffer::size_type   _sizeMark;
    };


    /**
     * Get size in bytes of the mandatory protocol message header.
     * @see MessageHeader
     * @return Size in bytes of the mandatory protocol message header.
     */
    static constexpr size_type headerSize() noexcept {
        // Note: can't use sizeof(MessageHeader) due to padding
        return  sizeof(MessageHeader::size) +
                sizeof(MessageHeader::type) +
                sizeof(MessageHeader::tag);
    }


public:

    P9Protocol(size_type maxMassageSize = MAX_MESSAGE_SIZE,
               const Solace::String& version = PROTOCOL_VERSION);


    size_type maxPossibleMessageSize() const noexcept {
        return _maxMassageSize;
    }

    size_type maxNegotiatedMessageSize() const noexcept {
        return _maxNegotiatedMessageSize;
    }

    size_type maxNegotiatedMessageSize(size_type newMessageSize);

    const Solace::String& getNegotiatedVersion() const noexcept {
        return _negotiatedVersion;
    }

    void setNegotiatedVersion(const Solace::String& version) noexcept {
        _negotiatedVersion = version;
    }

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
                               fid_type fid, Solace::uint64 offset, const Solace::ImmutableMemoryView& data);

    Tag createWalkRequest(Tag tag, Solace::ByteBuffer& dest,
                               fid_type fid, fid_type nfid, const Solace::Path& path);

    Tag createStatRequest(Tag tag, Solace::ByteBuffer& dest,
                               fid_type fid);

    Tag createWriteStatRequest(Tag tag, Solace::ByteBuffer& dest,
                               fid_type fid, const Stat& stat);


    /**
     * 9P2000.e extension
     */
    Tag createSessionRequest(Tag tag, Solace::ByteBuffer& dest,
                               const Solace::ImmutableMemoryView& key);

    Tag createShortReadRequest(Tag tag, Solace::ByteBuffer& dest,
                               fid_type fid, const Solace::Path& path);

    Tag createShortWriteRequest(Tag tag, Solace::ByteBuffer& dest,
                               fid_type fid, const Solace::Path& path, const Solace::ImmutableMemoryView& data);


    Solace::Result<MessageHeader, Solace::Error>
    parseMessageHeader(Solace::ByteBuffer& buffer) const;

    Solace::Result<Response, Solace::Error>
    parseMessage(const MessageHeader& header, Solace::ByteBuffer& data) const;

    Solace::Result<Request, Solace::Error>
    parseRequest(const MessageHeader& header, Solace::ByteBuffer& data) const;


protected:

    Solace::Result<Response, Solace::Error>
    parseNoDataResponse(const MessageHeader& header, Solace::ByteBuffer& data) const;

    Solace::Result<Response, Solace::Error>
    parseErrorResponse(const MessageHeader& header, Solace::ByteBuffer& data) const;

    Solace::Result<Response, Solace::Error>
    parseVersionResponse(const MessageHeader& header, Solace::ByteBuffer& data) const;

    Solace::Result<Response, Solace::Error>
    parseAuthResponse(const MessageHeader& header, Solace::ByteBuffer& data) const;

    Solace::Result<Response, Solace::Error>
    parseAttachResponse(const MessageHeader& header, Solace::ByteBuffer& data) const;

    Solace::Result<Response, Solace::Error>
    parseOpenResponse(const MessageHeader& header, Solace::ByteBuffer& data) const;

    Solace::Result<Response, Solace::Error>
    parseCreateResponse(const MessageHeader& header, Solace::ByteBuffer& data) const;

    Solace::Result<Response, Solace::Error>
    parseReadResponse(const MessageHeader& header, Solace::ByteBuffer& data) const;

    Solace::Result<Response, Solace::Error>
    parseWriteResponse(const MessageHeader& header, Solace::ByteBuffer& data) const;

    Solace::Result<Response, Solace::Error>
    parseStatResponse(const MessageHeader& header, Solace::ByteBuffer& data) const;

    Solace::Result<Response, Solace::Error>
    parseWalkResponse(const MessageHeader& header, Solace::ByteBuffer& data) const;

private:

    size_type   _maxMassageSize;
    size_type   _maxNegotiatedMessageSize;

    Solace::String _initialVersion;
    Solace::String _negotiatedVersion;
};

}  // end of namespace cadence
#endif  // CADENCE_PROTOCOLS_9P2000X_HPP
