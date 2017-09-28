/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Pro-actor style event loop
 *	@file		cadence/async/eventLoop.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_EVENTLOOP_HPP
#define CADENCE_ASYNC_EVENTLOOP_HPP

#include <solace/byteBuffer.hpp>
#include <solace/future.hpp>
#include <solace/io/selector.hpp>

#include "asio.hpp"


namespace cadence { namespace async {

/**
 * Event Loop.
 *
 * This package allows developers to use heigher level concept of the event loop to write reactive applicatoins.
 * Event loop abstacts data sources, polling all inputs and outputs and
 * triggers all event handlers when a channel is ready.
 *
 */
class EventLoop {
public:
    typedef Solace::IO::Selector::size_type size_type;

public:

    ~EventLoop() = default;

	EventLoop() = default;

    EventLoop(const EventLoop& rhs) = delete;

    EventLoop& operator= (const EventLoop& rhs) = delete;

    EventLoop& operator= (EventLoop&& rhs) noexcept {
        return swap(rhs);
    }

    EventLoop& swap(EventLoop& rhs) noexcept {
        using std::swap;
        return rhs;
    }

    bool poll();

    bool isStopped();

    void run();

    void runFor(int msec);

    void handler();

    void stop();

    void reset();

    asio::io_service& getIOService() noexcept {
        return _io_service;
    }

private:

	asio::io_service _io_service;
};


inline
asio::mutable_buffers_1 asio_buffer(Solace::ByteBuffer& dest, std::size_t size_in_bytes) ASIO_NOEXCEPT {
    return asio::buffer(dest.viewRemaining().slice(0, size_in_bytes).dataAddress(), size_in_bytes);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // SOLACE_IO_ASYNC_EVENTLOOP_HPP
