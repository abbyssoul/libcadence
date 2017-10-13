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
 * This bytes - or message frame - can be written into a network to communicate with other peers that
 * understand this protocol.
 *
 */
class P9Protocol {
public:
    typedef Solace::uint32 size_type;
    typedef Solace::uint16 Tag;
    typedef Solace::uint32 Fid;

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
    static const Fid NOFID;

    enum Consts {
        MAX_WELEM = 16
    };

    /**
     *  Flags for the mode field in Topen and Tcreate messages
     */
    enum class OpenMode : Solace::byte {
        READ   = 0,  // open read-only
        WRITE  = 1,  // open write-only
        RDWR   = 2,  // open read-write
        EXEC   = 3,  // execute (== read but check execute permission)
        TRUNC  = 16, // or'ed in (except for exec), truncate file first
        CEXEC  = 32, // or'ed in, close on exec
        RCLOSE = 64, // or'ed in, remove on close
    };

    /**
     * 9P message types
     */
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

    /**
     * Stat about a file on the server.
     */
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



    class P9Decoder {
    public:

        P9Decoder(Solace::ByteBuffer& src) :
            _src(src)
        {}

        P9Decoder& read(Solace::uint8* dest);
        P9Decoder& read(Solace::uint16* dest);
        P9Decoder& read(Solace::uint32* dest);
        P9Decoder& read(Solace::uint64* dest);
        P9Decoder& read(Solace::String* dest);
        P9Decoder& read(Qid* qid);
        P9Decoder& read(Stat* stat);
        P9Decoder& read(Solace::ImmutableMemoryView* data);
        P9Decoder& read(Solace::Path* path);

    private:
        Solace::ByteBuffer& _src;
    };

    /**
     * Common header that all messages have.
     */
    struct MessageHeader {
        size_type       size;   //!< Size of the message including size of the header and size field itself.
        MessageType     type;   //!< Type of the message. @see MessageType.
        Tag             tag;    //!< Message tag for concurent messages.
    };


    struct Request {

        struct Version {
            size_type       msize;
            Solace::String  version;
        };

        struct Auth {
            Fid        afid;
            Solace::String  uname;
            Solace::String  aname;
        };

        struct Flush {
            Tag        oldtag;
        };

        struct Attach {
            Fid             fid;
            Fid             afid;
            Solace::String  uname;
            Solace::String  aname;
        };

        struct Walk {
            Fid             fid;
            Fid             newfid;
            Solace::Path    path;
        };

        struct Open {
            Fid         fid;
            OpenMode    mode;
        };

        struct Create {
            Fid             fid;
            Solace::String  name;
            Solace::uint32  perm;
            OpenMode        mode;
        };

        struct Read {
            Fid             fid;
            Solace::uint64  offset;
            Solace::uint32  count;
        };

        struct Write {
            Fid        fid;
            Solace::uint64  offset;
            Solace::ImmutableMemoryView data;
        };

        struct Clunk {
            Fid        fid;
        };

        struct Remove {
            Fid        fid;
        };

        struct StatRequest {
            Fid        fid;
        };

        struct WStat {
            Fid        fid;
            Stat            stat;
        };


        struct Session {
            Solace::ImmutableMemoryView key;
        };

        struct SRead {
            Fid        fid;
            Solace::Path    path;
        };

        struct SWrite {
            Fid        fid;
            Solace::Path    path;
            Solace::ImmutableMemoryView data;
        };



        Request(MessageType rtype, Tag tag);
        Request(Request&& rhs);

        ~Request();

        Tag tag() const { return _tag; }

        MessageType type() const { return _type; }

        Version&        asVersion();
        Auth&           asAuth();
        Flush&          asFlush();
        Attach&         asAttach();
        Walk&           asWalk();
        Open&           asOpen();
        Create&         asCreate();
        Read&           asRead();
        Write&          asWrite();
        Clunk&          asClunk();
        Remove&         asRemove();
        StatRequest&    asStat();
        WStat&          asWstat();
        Session&        asSession();
        SRead&          asShortRead();
        SWrite&         asShortWrite();

    private:

        Tag             _tag;
        MessageType     _type;
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
    };


    /**
     * Helper class to build Request messages.
     */
    class RequestBuilder {
    public:

        RequestBuilder(Solace::ByteBuffer& dest) :
            _tag(1),
            _buffer(dest)
        {}

        Solace::ByteBuffer& buffer() {
            return _buffer;
        }

        Solace::ByteBuffer& build();

        /**
         * Set response message tag
         * @param value Tag of the response message.
         * @return Ref to this for a fluent interface.
         */
        RequestBuilder& tag(Tag value) {
            _tag = value;
            return (*this);
        }

        Tag tag() const { return _tag; }

        /**
         * Create a version request.
         * @param version Suggested protocol version.
         * @param maxMessageSize Suggest maximum size of the protocol message, including mandatory message header.
         * @return Ref to this for fluent interface.
         */
        RequestBuilder& version(const Solace::String& version = PROTOCOL_VERSION,
                                size_type maxMessageSize = MAX_MESSAGE_SIZE);
        RequestBuilder& auth(Fid afid, const Solace::String& userName, const Solace::String& attachName);
        RequestBuilder& flush(Tag oldTransation);
        RequestBuilder& attach(Fid fid, Fid afid,
                                const Solace::String& userName, const Solace::String& attachName);
        RequestBuilder& walk(Fid fid, Fid nfid, const Solace::Path& path);
        RequestBuilder& open(Fid fid, OpenMode mode);
        RequestBuilder& create(Fid fid,
                                const Solace::String& name,
                                Solace::uint32 permissions,
                                OpenMode mode);
        RequestBuilder& read(Fid fid, Solace::uint64 offset, size_type count);
        RequestBuilder& write(Fid fid, Solace::uint64 offset, const Solace::ImmutableMemoryView& data);
        RequestBuilder& clunk(Fid fid);
        RequestBuilder& remove(Fid fid);
        RequestBuilder& stat(Fid fid);
        RequestBuilder& writeStat(Fid fid, const Stat& stat);

        /* 9P2000.e extention */
        RequestBuilder& session(const Solace::ImmutableMemoryView& key);
        RequestBuilder& shortRead(Fid rootFid, const Solace::Path& path);
        RequestBuilder& shortWrite(Fid rootFid, const Solace::Path& path, const Solace::ImmutableMemoryView& data);

    private:
        Tag                     _tag;
        Solace::ByteBuffer&     _buffer;
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

        ResponseBuilder(Solace::ByteBuffer& dest) :
            _tag(1),
            _buffer(dest)
        {}

        Solace::ByteBuffer& buffer() {
            return _buffer;
        }

        Solace::ByteBuffer& build();

        /**
         * Set response message tag
         * @param value Tag of the response message.
         * @return Ref to this for a fluent interface.
         */
        ResponseBuilder& tag(Tag value) {
            _tag = value;
            return (*this);
        }

        Tag tag() const { return _tag; }

        ResponseBuilder& version(const Solace::String& version, size_type maxMessageSize = MAX_MESSAGE_SIZE);
        ResponseBuilder& auth(const Qid& qid);
        ResponseBuilder& error(const Solace::String& message);
        ResponseBuilder& flush();
        ResponseBuilder& attach(const Qid& qid);
        ResponseBuilder& walk(const Solace::Array<Qid>& qids);
        ResponseBuilder& open(const Qid& qid, size_type iounit);
        ResponseBuilder& create(const Qid& qid, size_type iounit);
        ResponseBuilder& read(const Solace::ImmutableMemoryView& data);
        ResponseBuilder& write(size_type iounit);
        ResponseBuilder& clunk();
        ResponseBuilder& remove();
        ResponseBuilder& stat(const Stat& value);
        ResponseBuilder& wstat();

        /* 9P2000.e extention */
        ResponseBuilder& session();
        ResponseBuilder& shortRead(const Solace::ImmutableMemoryView& data);
        ResponseBuilder& shortWrite(size_type iounit);

    private:
        Tag                     _tag;
        Solace::ByteBuffer&     _buffer;
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

    Solace::Result<MessageHeader, Solace::Error>
    parseMessageHeader(Solace::ByteBuffer& buffer) const;

    Solace::Result<Response, Solace::Error>
    parseResponse(const MessageHeader& header, Solace::ByteBuffer& data) const;

    Solace::Result<Request, Solace::Error>
    parseRequest(const MessageHeader& header, Solace::ByteBuffer& data) const;

private:

    size_type       _maxMassageSize;
    size_type       _maxNegotiatedMessageSize;

    Solace::String  _initialVersion;
    Solace::String  _negotiatedVersion;
};

}  // end of namespace cadence
#endif  // CADENCE_PROTOCOLS_9P2000X_HPP
