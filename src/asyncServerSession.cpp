/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/

#include "cadence/asyncServer.hpp"
#include "cadence/protocols/9p2000x.hpp"


#include <iostream>  // std::cout
#include <iomanip>   // std::setw


using namespace Solace;
using namespace cadence;
using namespace cadence::async;


static const String kThisDir(".");
static const String kParentDir("..");


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


P9Protocol::Qid nodeToQid(std::shared_ptr<AsyncServer::Node> node) {
    P9Protocol::Qid qid;
    qid.type = node->isWalkable()
            ? static_cast<byte>(P9Protocol::QidType::DIR)
            : static_cast<byte>(P9Protocol::QidType::FILE);

    qid.path = reinterpret_cast<uint64>(node.get());
    qid.version = node->getVersion();

    return qid;
}


P9Protocol::Stat nodeStats(const String& name, const std::shared_ptr<AsyncServer::Node>& node) {
    ByteBuffer emptyBuffer;
    P9Protocol::Encoder encoder(emptyBuffer);

    P9Protocol::Stat stats;
    stats.name = name;
    stats.qid = nodeToQid(node);
    stats.mode = node->isWalkable()
            ? static_cast<uint32>(P9Protocol::DirMode::DIR)
            : static_cast<uint32>(P9Protocol::DirMode::TMP);

    stats.size = encoder.protocolSize(stats) - sizeof(stats.size);

    return stats;
}



Result<std::shared_ptr<AsyncServer::Node>, Error>
walk(const std::shared_ptr<AsyncServer::DirectoryNode>& root, const Path& path, Array<P9Protocol::Qid>& qids) {
    std::shared_ptr<AsyncServer::Node> node(root);

    Array<P9Protocol::Qid>::size_type i = 0;
    for (const auto& pathComponent : path) {
        auto walkResult = node->walk(pathComponent);

        if (walkResult) {
            node = walkResult.unwrap();
            qids[i] = nodeToQid(node);
        } else {
            return Err(walkResult.moveError());
        }
    }

    return Ok(std::move(node));
}



class AsyncServer::Session :
        public std::enable_shared_from_this<Session>
{
public:

    ~Session() {
        std::cout << "Session from " << _socket.getRemoteEndpoint().toString() << " destroyed." << std::endl;
    }

    Session(DirectoryNode& serverRoot, TcpSocket&& socket, ByteBuffer&& buffer, ByteBuffer&& readbuf) :
        _socket(std::move(socket)),
        _buffer(std::move(buffer)),
        _readBuffer(std::move(readbuf)),
        _serverRoot(serverRoot)
    {
    }

    void start() {
        doRead();
    }

private:

    void onVersionRequest(const P9Protocol::Request::Version& req, P9Protocol::ResponseBuilder& responseBuilder) {
        const auto minMessageSize = std::min(_protocol.maxNegotiatedMessageSize(), req.msize);
        _protocol.maxNegotiatedMessageSize(minMessageSize);

        String negotiatedVersion;

        // If we can only server sub-version
        if (req.version.startsWith(_protocol.PROTOCOL_VERSION))
            negotiatedVersion = _protocol.PROTOCOL_VERSION;
        else if (_protocol.PROTOCOL_VERSION.startsWith(req.version))
            negotiatedVersion = req.version;
        else
            negotiatedVersion = _protocol.UNKNOWN_PROTOCOL_VERSION;

        responseBuilder.version(negotiatedVersion, _protocol.maxNegotiatedMessageSize());
    }

    void onAuthRequest(const P9Protocol::Request::Auth& req, P9Protocol::ResponseBuilder& responseBuilder) {

        _uname = req.uname;

        // TODO(abbyssoul): Implement auth middlewere
        responseBuilder.error("Auth not supported");
    }

    void onAttachRequest(const P9Protocol::Request::Attach& req, P9Protocol::ResponseBuilder& responseBuilder) {
        std::shared_ptr<AsyncServer::Node> attachementPoint;

        if (req.aname.empty()) {
            attachementPoint.reset(&_serverRoot, [](AsyncServer::DirectoryNode*){});
        } else {
            auto res = _serverRoot.walk(req.aname);

            if (res) {
                auto attNode = res.unwrap();

                if (attNode->isWalkable()) {
                    attachementPoint = std::static_pointer_cast<AsyncServer::DirectoryNode>(attNode);
                } else {
                    responseBuilder.error("File not found");

                    return;
                }
            } else {
                responseBuilder.error(res.getError());

                return;
            }
        }

        P9Protocol::Qid qid = nodeToQid(attachementPoint);

        _openedNodes.emplace(req.fid, std::move(attachementPoint));

        responseBuilder.attach(qid);
    }


    void onWalkRequest(const P9Protocol::Request::Walk& req, P9Protocol::ResponseBuilder& responseBuilder) {
        auto attachmentIt = _openedNodes.find(req.fid);
        if (attachmentIt == _openedNodes.end()) {
            responseBuilder.error("Not attached");
            return;
        }

        if (!attachmentIt->second->isWalkable()) {
            responseBuilder.error("fid is not a directory");
            return;
        }

        auto rootDir = std::static_pointer_cast<AsyncServer::DirectoryNode>(attachmentIt->second);
        // FIXME: Protection from a user supplied path length
        Array<P9Protocol::Qid> qids(req.path.getComponentsCount());
        auto walkResult = walk(rootDir, req.path, qids);

        if (walkResult) {
            _openedNodes[req.newfid] = walkResult.unwrap();
            responseBuilder.walk(qids);
        } else {
            responseBuilder.error(walkResult.getError());
            return;
        }
    }

    void onOpenRequest(const P9Protocol::Request::Open& req, P9Protocol::ResponseBuilder& responseBuilder) {
        auto nodeIt = _openedNodes.find(req.fid);
        if (nodeIt == _openedNodes.end()) {
            responseBuilder.error("Not attached");
            return;
        }

        _readBuffer.clear();
        auto readResult = nodeIt->second->open(_uname, static_cast<byte>(req.mode));
        if (readResult) {
            P9Protocol::Qid qid = nodeToQid(nodeIt->second);

            responseBuilder.open(qid, 0);
        } else {
            responseBuilder.error(readResult.getError());
        }
    }

    void onStatRequest(const P9Protocol::Request::StatRequest& req, P9Protocol::ResponseBuilder& responseBuilder) {
        auto nodeIt = _openedNodes.find(req.fid);
        if (nodeIt == _openedNodes.end()) {
            responseBuilder.error("Not attached");
            return;
        }

        auto readResult = nodeIt->second->getStats();
        if (readResult) {
            responseBuilder.stat(nodeStats(kThisDir, nodeIt->second));
        } else {
            responseBuilder.error(readResult.getError());
        }
    }

    void onReadRequest(const P9Protocol::Request::Read& req, P9Protocol::ResponseBuilder& responseBuilder) {
        auto nodeIt = _openedNodes.find(req.fid);
        if (nodeIt == _openedNodes.end()) {
            responseBuilder.error("Not attached");
            return;
        }

        _readBuffer.clear();
        auto readResult = nodeIt->second->read(req.count, req.offset, _readBuffer);
        if (readResult)
            responseBuilder.read(_readBuffer.viewWritten());
        else
            responseBuilder.error(readResult.getError());
    }


    void onFlushRequest(const P9Protocol::Request::Flush& /*req*/, P9Protocol::ResponseBuilder& responseBuilder) {
        responseBuilder.flush();
    }


    void onClankRequest(const P9Protocol::Request::Clunk& req, P9Protocol::ResponseBuilder& responseBuilder) {
        auto nodeIt = _openedNodes.find(req.fid);
        if (nodeIt == _openedNodes.end()) {
            responseBuilder.error("Not attached");
            return;
        }

        _openedNodes.erase(nodeIt);
        responseBuilder.clunk();
    }


    void dispatchRequest(P9Protocol::Request&& req) {

        _buffer.clear();
        P9Protocol::ResponseBuilder responseBuilder(_buffer);
        responseBuilder.tag(req.tag());

        switch (req.type()) {
        case P9Protocol::MessageType::TVersion:     onVersionRequest(req.asVersion(), responseBuilder); break;
        case P9Protocol::MessageType::TAuth:        onAuthRequest(req.asAuth(), responseBuilder); break;
        case P9Protocol::MessageType::TAttach:      onAttachRequest(req.asAttach(), responseBuilder); break;
        case P9Protocol::MessageType::TClunk:       onClankRequest(req.asClunk(), responseBuilder); break;
        case P9Protocol::MessageType::TOpen:        onOpenRequest(req.asOpen(), responseBuilder); break;
//        case P9Protocol::MessageType::TCreate:        onCreateRequest(req.asCreate(), responseBuilder); break;
        case P9Protocol::MessageType::TFlush:       onFlushRequest(req.asFlush(), responseBuilder); break;
        case P9Protocol::MessageType::TStat:        onStatRequest(req.asStat(), responseBuilder); break;

        case P9Protocol::MessageType::TRead:        onReadRequest(req.asRead(), responseBuilder); break;
        case P9Protocol::MessageType::TWalk:        onWalkRequest(req.asWalk(), responseBuilder); break;
        default:
            responseBuilder.error("Unsupported request");
            break;
        }

        auto& responseData = responseBuilder.build();
        std::cout << "← [" << std::setw(5) << responseData.remaining() << "] " <<
                     responseBuilder.type() << " " <<
                     responseBuilder.tag() <<
                     std::endl;

        doWrite(responseData);
    }


    void doRead() {
        auto self(shared_from_this());

        _socket.asyncRead(_buffer.clear(), _protocol.headerSize())
                .then([this, self]() {

                    _protocol.parseMessageHeader(_buffer.flip())
                            .then([this, self](P9Protocol::MessageHeader&& header) {
                                const auto payloadSize = header.size - _protocol.headerSize();

                                // Log incomming message:
                                std::cout << "→ [" << std::setw(5) << header.size << "] " <<
                                             header.type << " " <<
                                             header.tag <<
                                             std::endl;


                                _socket.asyncRead(_buffer.clear(), payloadSize)
                                    .then([msgHeader = std::move(header), self = std::move(self), this]() {

                                        _protocol.parseRequest(msgHeader, _buffer.flip())
                                            .then([this](P9Protocol::Request&& req) {
                                                dispatchRequest(std::move(req));
                                            });
                                })
                                .onError([txId = header.tag](Error&& e) {
                                    std::cout << "Failed to read the message payload: " << e.toString() << std::endl;
                                });
                    }).orElse([](Error&& e){
                        std::cout << "Failed to parse message header: " << e.toString() << std::endl;
                    });
                })
                .onError([](Error&& e) {
                    std::cout << "Failed to read data from the socket: " << e.toString() <<
                                 " - closing connection." <<
                                 std::endl;
                });
    }


    void doWrite(ByteBuffer& data) {
        auto self(shared_from_this());

        _socket.asyncWrite(data)
                .then([self]() {
                    self->doRead();
                });
    }

private:

    P9Protocol  _protocol;

    TcpSocket   _socket;
    ByteBuffer  _buffer;
    ByteBuffer  _readBuffer;    // Temporary

    // TODO(abbyssoul): Better credential handling:
    String      _uname;

    AsyncServer::DirectoryNode&                     _serverRoot;
    std::map<P9Protocol::Fid, std::shared_ptr<AsyncServer::Node>>  _openedNodes;
};



std::shared_ptr<AsyncServer::Session>
AsyncServer::spawnSession() {
    // TODO(abbyssoul): Pick a free buffer from a pre-allocated pool or reject connection if non avaliable.
    ByteBuffer msgBuf(_memManager.create(P9Protocol::MAX_MESSAGE_SIZE));
    ByteBuffer readBuf(_memManager.create(P9Protocol::MAX_MESSAGE_SIZE));

    auto session = std::make_shared<Session>(_root, std::move(_socket), std::move(msgBuf), std::move(readBuf));
    session->start();

    return session;
}
