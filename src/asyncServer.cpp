/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/

#include "cadence/asyncServer.hpp"


using namespace Solace;
using namespace cadence;
using namespace cadence::async;


void
doAcceptSession(async::Acceptor& acceptor, AsyncServer::AcceptHandler& handler) {
    acceptor.asyncAccept()
            .then([&](StreamSocket&& socket) {
                if (handler) {
                    handler(std::move(socket));
                }

                // Keep on accepting other sessions
                if (acceptor.isOpen()) {
                    doAcceptSession(acceptor, handler);
                }
          });
}


void
AsyncServer::stop() {
    _acceptor.close();
}


Solace::Result<void, Solace::Error>
AsyncServer::startListen(NetworkEndpoint const& endpoint) {
    return _acceptor.open(endpoint)
                .then([this]() {
                    doAcceptSession(_acceptor, _connectionHandler);
                });
}


