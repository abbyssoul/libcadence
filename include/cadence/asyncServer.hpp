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
#include "async/streamsocket.hpp"
#include "async/acceptor.hpp"

#include <solace/result.hpp>

#include <functional>  // std::function


namespace cadence {

/**
 * Asynchronious data server that server.
 */
class AsyncServer {
public:
    using AcceptHandler = std::function<void(async::StreamSocket&&)>;

    // Non-copible
    AsyncServer(AsyncServer const&) = delete;
    AsyncServer& operator= (AsyncServer const&) = delete;

    // Movable:
    AsyncServer(AsyncServer&&) noexcept = default;
    AsyncServer& operator= (AsyncServer&&) = default;

    template<typename CB>
    AsyncServer(async::EventLoop& eventLoop, CB&& cb) :
        _acceptor(eventLoop),
        _connectionHandler(std::forward<CB>(cb))
    {}

    Solace::Result<void, Solace::Error> startListen(NetworkEndpoint const& endpoint);

    void stop();

private:

    async::Acceptor _acceptor;
    AcceptHandler   _connectionHandler;
};

}  // End of namespace cadence
#endif  // CADENCE_ASYNCSERVER_HPP
