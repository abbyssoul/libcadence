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


using namespace Solace;
using namespace cadence;
using namespace cadence::async;



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



Result<void, Error>
AsyncServer::DirectoryNode::mount(const Solace::String& pathSegment, std::unique_ptr<Node>&& node) {
    auto it = _mounts.find(pathSegment);
    if (it == _mounts.end()) {
        _mounts.emplace(pathSegment, std::move(node));

        return Ok();
    } else {
        return Err(Error("Can't mount: path already exist"));
    }
}


Result<std::shared_ptr<AsyncServer::Node>, Error>
AsyncServer::DirectoryNode::walk(const Solace::String& pathSegment) {
    auto it = _mounts.find(pathSegment);
    if (it == _mounts.end())
        return Err(Error("not found"));

    return Ok(it->second);
}



class Session :
        public std::enable_shared_from_this<Session>
{
public:

    ~Session() {
        std::cout << "Session from " << _socket.getRemoteEndpoint().toString() << " destroyed." << std::endl;
    }

    Session(TcpSocket&& socket, ByteBuffer&& buffer) :
        _socket(std::move(socket)),
        _buffer(std::move(buffer))
    {
    }

    void start() {
        doRead();
    }

private:

    void errorRequest(ByteBuffer& dest, P9Protocol::Request&& req) {
        P9Protocol::ResponseBuilder(dest)
                .tag(req.tag())
                .error("Unsupported request");
    }

    void onVersionRequest(ByteBuffer& dest, P9Protocol::Tag tag, const P9Protocol::Request::Version& req) {

        const auto minMessageSize = std::min(_protocol.maxNegotiatedMessageSize(), req.msize);
        _protocol.maxNegotiatedMessageSize(minMessageSize);

        P9Protocol::ResponseBuilder(dest)
                .tag(tag)
                .version(_protocol.getNegotiatedVersion(), _protocol.maxNegotiatedMessageSize());
    }

    void onAuthRequest(ByteBuffer& dest, P9Protocol::Tag tag, const P9Protocol::Request::Auth& req) {
        // TODO: Implement auth middlewere
        P9Protocol::ResponseBuilder(dest)
                .tag(tag)
                .error("Auth not supported");
    }

    void onAttachRequest(ByteBuffer& dest, P9Protocol::Tag tag, const P9Protocol::Request::Attach& req) {
        // TODO: Implement attchement
        P9Protocol::Qid qid;
        qid.type = static_cast<byte>(P9Protocol::QidType::DIR);

        P9Protocol::ResponseBuilder(dest)
                .tag(tag)
                .attach(qid);
    }

    void onWalkRequest(ByteBuffer& dest, P9Protocol::Tag tag, const P9Protocol::Request::Walk& req) {
        // TODO: Implement attchement

        // FIXME: Protection from a user supplied path lenght
        Array<P9Protocol::Qid> qids(req.path.getComponentsCount());

        P9Protocol::ResponseBuilder(dest)
                .tag(tag)
                .walk(qids);
    }


    void dispatchRequest(P9Protocol::Request&& req) {
        std::cout << "-> " << req.type() << std::endl;

        _buffer.clear();

        switch (req.type()) {
        case P9Protocol::MessageType::TVersion:     onVersionRequest(_buffer, req.tag(), req.asVersion()); break;
        case P9Protocol::MessageType::TAuth:        onAuthRequest(_buffer, req.tag(), req.asAuth()); break;
        case P9Protocol::MessageType::TAttach:      onAttachRequest(_buffer, req.tag(), req.asAttach()); break;
        case P9Protocol::MessageType::TWalk:        onWalkRequest(_buffer, req.tag(), req.asWalk()); break;
        default:
            errorRequest(_buffer, std::move(req));
            break;
        }


        doWrite(_buffer.flip());
    }

    void doRead() {
        auto self(shared_from_this());

        _socket.asyncRead(_buffer.clear(), _protocol.headerSize())
                .then([this, self]() {

                    _protocol.parseMessageHeader(_buffer.flip())
                            .then([this, self](P9Protocol::MessageHeader&& header) {
                        const auto payloadSize = header.size - _protocol.headerSize();

                        _socket.asyncRead(_buffer.clear(), payloadSize)
                                .then([msgHeader = std::move(header), self=std::move(self), this]() {

                                    _protocol.parseRequest(msgHeader, _buffer.flip())
                                            .then([this](P9Protocol::Request&& req) {
                                                dispatchRequest(std::move(req));
                                            });
                                })
                                .onError([txId = header.tag, this](Error&& e) {
                                    std::cout << "Failed to read the message payload: " << e.toString() << std::endl;
                                });
                    }).orElse([](Error&& e){
                        std::cout << "Failed to parse message header: " << e.toString() << std::endl;
                    });
                });
    }

    void doWrite(ByteBuffer& data) {
        auto self(shared_from_this());

        _socket.asyncWrite(data)
                .then([this, self]() {
                    doRead();
                });
    }

private:

    TcpSocket   _socket;
    ByteBuffer  _buffer;

    P9Protocol  _protocol;
};

AsyncServer::AsyncServer(async::EventLoop& eventLoop, Solace::MemoryManager& memManager) :
    _memManager(memManager),
    _acceptor(eventLoop),
    _socket(eventLoop)
{}


Result<void, Error>
AsyncServer::startListen(const IPEndpoint& endpoint) {
    std::cout << "Server listening at " << endpoint.toString() << std::endl;

    return _acceptor.open(endpoint, 80)
            .then([this]() {
                doAccept();
            });
}


void
AsyncServer::stop() {
    std::cout << "Server stop listening" << std::endl;

    _acceptor.close();
}


void
AsyncServer::doAccept() {
    if (_acceptor.isClosed())
        return;

    _acceptor.asyncAccept(_socket)
            .then([this]() {
                std::cout << "New connection from " << _socket.getRemoteEndpoint().toString() << std::endl;

                ByteBuffer buf(_memManager.create(P9Protocol::MAX_MESSAGE_SIZE));
                std::make_shared<Session>(std::move(_socket), std::move(buf))->start();

                doAccept();
          });
}


Result<void, Error>
AsyncServer::mount(Path& path, std::unique_ptr<Node>&& newNode) {
    if (path.empty())
        return Err(Error("Path is empty"));

    Node* node = &_root;
    for (Path::size_type i = 0; i < path.getComponentsCount(); ++i) {
        const auto& pathComponent = path.getComponent(i);
        const bool isLastComponent = (i == path.getComponentsCount() - 1);

        if (isLastComponent) {
            return node->mount(pathComponent, std::move(newNode));
        } else {
            const auto walkResult = _root.walk(pathComponent);
            if (walkResult) {
                node = walkResult.unwrap().get();
            } else {
                return Err(walkResult.getError());
            }
        }
    }

}
