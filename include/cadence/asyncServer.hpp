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

#include <solace/path.hpp>

#include <map>

namespace cadence {

class AsyncServer {
public:

    /**
     * Base class for Nodes served by the server.
     * Node is an element of the hierarchy. It can be a resource such as file or a directory.
     * Files can be a data blobs such as files of a disk or synthetic resources (like proc fs)
     */
    class Node {
    public:
        virtual ~Node() = default;

        virtual Solace::Result<void, Solace::Error>
        mount(const Solace::String& pathSegment, std::unique_ptr<Node>&& node) = 0;

        virtual Solace::Result<std::shared_ptr<Node>, Solace::Error>
        walk(const Solace::String& pathSegment) = 0;
    };


    class DirectoryNode : public Node {
    public:
        Solace::Result<void, Solace::Error>
        mount(const Solace::String& pathSegment, std::unique_ptr<Node>&& node) override;

        Solace::Result<std::shared_ptr<Node>, Solace::Error>
        walk(const Solace::String& pathSegment) override;

    private:

        std::map<Solace::String, std::shared_ptr<Node>> _mounts;
    };

public:

    AsyncServer(async::EventLoop& eventLoop, Solace::MemoryManager& memManager);

    Solace::Result<void, Solace::Error> configure();

    Solace::Result<void, Solace::Error> startListen(const IPEndpoint& endpoint);
    void stop();

    Solace::Result<void, Solace::Error> mount(Solace::Path& path, std::unique_ptr<Node>&& r);

protected:

    void doAccept();

private:

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
