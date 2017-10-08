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



using namespace Solace;
using namespace cadence;



const P9Protocol::size_type P9Protocol::MAX_MESSAGE_SIZE = 8*1024;      // 8k should be enough for everyone, am I right?
const String                P9Protocol::PROTOCOL_VERSION = "9P2000.e";  // By default we want to talk via 9P2000.e proc
const P9Protocol::Tag       P9Protocol::NO_TAG = static_cast<P9Protocol::Tag>(~0);
const P9Protocol::Fid  P9Protocol::NOFID = static_cast<P9Protocol::Fid>(~0);


// static const char* UNKNOWN_PROTOCOL_VERSION = "unknown";


class P9Decoder {
public:

    P9Decoder(ByteBuffer& src) :
        _src(src)
    {}

    P9Decoder& read(uint8* dest) {
        _src.readLE(*dest);
        return *this;
    }

    P9Decoder& read(uint16* dest) {
        _src.readLE(*dest);
        return *this;
    }

    P9Decoder& read(uint32* dest) {
        _src.readLE(*dest);
        return *this;
    }

    P9Decoder& read(uint64* dest) {
        _src.readLE(*dest);
        return *this;
    }

    P9Decoder& read(String* dest) {
        uint16 dataSize = 0;
        _src.readLE(dataSize);

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



Result<P9Protocol::Response, Error>
parseNoDataResponse(const P9Protocol::MessageHeader& header, ByteBuffer& SOLACE_UNUSED(data)) {
    P9Protocol::Response fcall(header.type, header.tag);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
parseErrorResponse(const P9Protocol::MessageHeader& SOLACE_UNUSED(header), ByteBuffer& data) {
    String errorMessage;

    P9Decoder(data)
            .read(&errorMessage);

    return Err(Error(errorMessage.c_str()));
}


Result<P9Protocol::Response, Error>
parseVersionResponse(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.version.msize)
            .read(&fcall.version.version);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
parseAuthResponse(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.auth.qid);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
parseAttachResponse(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.attach.qid);

    return Ok(std::move(fcall));
}



Result<P9Protocol::Response, Error>
parseOpenResponse(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.open.qid)
            .read(&fcall.open.iounit);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
parseCreateResponse(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.create.qid)
            .read(&fcall.open.iounit);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
parseReadResponse(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.read.data);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
parseWriteResponse(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.write.count);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
parseStatResponse(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Response fcall(header.type, header.tag);

    P9Decoder(data)
            .read(&fcall.stat);

    return Ok(std::move(fcall));
}


Result<P9Protocol::Response, Error>
parseWalkResponse(const P9Protocol::MessageHeader& header, ByteBuffer& data) {
    P9Protocol::Response fcall(header.type, header.tag);

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
    buffer.readLE(header.size);

    // Sanity checks:
    // It is a serious error if server responded with the message of a size bigger than negotiated one.
//    Solace::assertIndexInRange(header.size, headerSize(), maxNegotiatedMessageSize());
    if (header.size < headerSize())
        return Err(Error("Ill-formed message: Declared frame size less than header"));
    if (header.size > maxNegotiatedMessageSize())
        return Err(Error("Ill-formed message: Declared frame size greater than negotiated message size"));

    // Read message type:
    byte messageBytecode;
    buffer.readLE(messageBytecode);
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
    buffer.readLE(header.tag);

    return Ok(header);
}


Result<P9Protocol::Response, Error>
P9Protocol::parseResponse(const MessageHeader& header, ByteBuffer& data) const {
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
    // Just paranoid about huge messages exciding frame size getting through.
    if (header.size > maxNegotiatedMessageSize())
        return Err(Error("Ill-formed message: Declared frame size greater than negotiated message size"));

    const auto expectedData = header.size - headerSize();

    // Message data sanity check
    // Make sure we have been given enough data to read a message as requested in the message size.
    if (expectedData > data.remaining())
        return Err(Error("Ill-formed message: Declared frame size larger than message data received"));

    // Make sure there is no extra unexpected data in the buffer.
    if (expectedData < data.remaining())
        return Err(Error("Ill-formed message: Declared frame size less than message data received"));

    switch (header.type) {
    case MessageType::TVersion: return parseVersionRequest(header,      data);
    case MessageType::TAuth:    return parseAuthRequest(header,         data);
    case MessageType::TFlush:   return parseFlushRequest(header,        data);
    case MessageType::TAttach:  return parseAttachRequest(header,       data);
    case MessageType::TWalk:    return parseWalkRequest(header,         data);
    case MessageType::TOpen:    return parseOpenRequest(header,         data);
    case MessageType::TCreate:  return parseCreateRequest(header,       data);
    case MessageType::TRead:    return parseReadRequest(header,         data);
    case MessageType::TWrite:   return parseWriteRequest(header,        data);
    case MessageType::TClunk:   return parseClunkRequest(header,        data);
    case MessageType::TRemove:  return parseRemoveRequest(header,       data);
    case MessageType::TStat:    return parseStatRequest(header,         data);
    case MessageType::TWStat:   return parseWStatRequest(header,        data);
    /* 9P2000.e extension messages */
    case MessageType::TSession: return parseSessionRequest(header,      data);
    case MessageType::TSRead:   return parseShortReadRequest(header,    data);
    case MessageType::TSWrite:  return parseShortWriteRequest(header,   data);

    default:
        return Err(Error("Failed to parse responce message: Unsupported message type"));
    }
}

P9Protocol::size_type P9Protocol::maxNegotiatedMessageSize(size_type newMessageSize) {
    Solace::assertIndexInRange(newMessageSize, 0, maxPossibleMessageSize() + 1);
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
