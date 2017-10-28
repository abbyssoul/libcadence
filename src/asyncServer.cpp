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




AsyncServer::Node::~Node() {
}

Result<std::shared_ptr<AsyncServer::Node>, Error>
AsyncServer::Node::walk(const String& ) {
    return Err(Error("Not a directory"));
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

Result<void, Error>
AsyncServer::DirectoryNode::open(const String& uname, byte mode) {
    // TODO(abbyssoul): Check ACL if user identified by uname can has access.

    return Ok();
}


Result<void, Error>
AsyncServer::DirectoryNode::getStats() {
    return Ok();
}


// Defined in asyncServerSession:
P9Protocol::Stat nodeStats(const String& name, const std::shared_ptr<AsyncServer::Node>& node);


Result<void, Error>
AsyncServer::DirectoryNode::read(uint32 count, uint64 offset, ByteBuffer& buffer) {
    P9Protocol::Encoder encoder(buffer);

    uint64 bytesTraversed = 0;
    uint32 bytesEncoded = 0;

    for (auto entry : _mounts) {
        P9Protocol::Stat stat(nodeStats(entry.first, entry.second));

        const auto protoSize = encoder.protocolSize(stat);
        // Keep count of how many data we have traversed.
        bytesTraversed += protoSize;
        if (bytesTraversed <= offset)  // Client is only interested in data pass the offset.
            continue;

        // Keep track of much data will end up in a buffer to prevent overflow.
        bytesEncoded += protoSize;
        if (bytesEncoded > count)
            break;

        // Only encode the data if we have some room left, as specified by 'count' arg.
        encoder.encode(stat);
    }

    return Ok();
}


Result<void, Error>
AsyncServer::DirectoryNode::write(ImmutableMemoryView data, uint64 offset) {
    return Err(Error("Write not allowed"));
}



AsyncServer::AsyncServer(async::EventLoop& eventLoop, Solace::MemoryManager& memManager) :
    _memManager(memManager),
    _acceptor(eventLoop),
    _socket(eventLoop)
{}


Result<void, Error>
AsyncServer::startListen(const IPEndpoint& endpoint) {
    std::cout << "Server listening at " << endpoint.toString() << std::endl;

    auto testNode = std::make_unique<DirectoryNode>();
    auto testNode1 = std::make_unique<DirectoryNode>();
    auto testNode2 = std::make_unique<DirectoryNode>();
    testNode1->mount("internal", std::move(testNode2));

    _root.mount("pathSomewhere", std::move(testNode));
    _root.mount("non_empty_dir", std::move(testNode1));

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

                spawnSession();

                // Keep on accepting other sessions
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
