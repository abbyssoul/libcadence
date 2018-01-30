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




AsyncServer::AsyncServer(async::EventLoop& eventLoop, Solace::MemoryManager& memManager) :
    _memManager(memManager),
    _acceptor(eventLoop),
    _socket(eventLoop)
{}


Result<void, Error>
AsyncServer::startListen(const IPEndpoint& endpoint) {
    std::cout << "Server listening at " << endpoint.toString() << std::endl;

//    auto testNode = std::make_unique<DirectoryNode>();
//    auto testNode1 = std::make_unique<DirectoryNode>();
//    auto testNode2 = std::make_unique<DirectoryNode>();
//    testNode1->mount("internal", std::move(testNode2));

//    _root.mount("pathSomewhere", std::move(testNode));
//    _root.mount("non_empty_dir", std::move(testNode1));

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
AsyncServer::mount(const Path& path, std::shared_ptr<Node>&& newNode) {
    if (path.empty()) {
        return Err(Error("Path is empty"));
    }

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
