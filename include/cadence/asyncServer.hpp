/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Async Resource Server
 *	@file		cadence/asyncServer.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNCSERVER_HPP
#define CADENCE_ASYNCSERVER_HPP


#include "async/eventloop.hpp"
#include "async/tcpsocket.hpp"
#include "ipendpoint.hpp"
#include "serviceNodes.hpp"

#include <solace/path.hpp>


namespace cadence {

/**
 * Asynchronious data server that server serviceNodes via resource protocol.
 */
class AsyncServer {
public:

    AsyncServer(async::EventLoop& eventLoop, Solace::MemoryManager& memManager);

    Solace::Result<void, Solace::Error> configure();

    Solace::Result<void, Solace::Error> startListen(const IPEndpoint& endpoint);
    void stop();

    Solace::Result<void, Solace::Error> mount(const Solace::Path& path, std::shared_ptr<Node>&& r);

protected:

    void doAccept();

    class Session;
    std::shared_ptr<Session> spawnSession();

private:

    /// Memory manager used to allocate memory for new sessions.
    Solace::MemoryManager&          _memManager;

    async::TcpAcceptor     _acceptor;
    async::TcpSocket       _socket;

    /// A resource being server by the server
    /// std::unique_ptr<AuthHandler>    _authHandler;

    /// Root directory of the server where all other nodes are mounted to.
    DirectoryNode        _root;
};

}  // End of namespace cadence
#endif  // CADENCE_ASYNCSERVER_HPP
