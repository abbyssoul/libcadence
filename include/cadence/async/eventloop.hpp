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

#include <solace/types.hpp>
#include <memory>  // std::unique_ptr<>


namespace cadence::async {

/**
 * Event loop.
 *
 * This package allows developers to use heigher level concept of the event loop to write reactive applicatoins.
 * Event loop abstacts data sources, polling all inputs and outputs and
 * triggers all event handlers when a channel is ready.
 *
 */
class EventLoop {
public:

    using size_type = Solace::uint32;

public:
    ~EventLoop();  // Note: Must be provided for pimpl destructor

    EventLoop();

    EventLoop(EventLoop const& rhs) = delete;
    EventLoop& operator= (EventLoop const& rhs) = delete;

    EventLoop& operator= (EventLoop&& rhs) noexcept {
        return swap(rhs);
    }

    EventLoop& swap(EventLoop& rhs) noexcept {
        using std::swap;
        swap(_pimpl, rhs._pimpl);

        return rhs;
    }

    size_type poll();

    bool isStopped() const noexcept;

    /**
     * Run event processing loop until `stop()` is called or there is no more jobs queued.
     */
    void run();

    /**
     * This call blocks until all work has finished and there are no more handlers to be dispatched,
     * until the event loop has been stopped, or until the specified duration has elapsed.
     *
     * @param msec The duration for which the call may block in milliseconds.
     */
    void runFor(int msec);

    /**
     * Stop event processing loop.
     */
    void stop();

    void reset();

    void* getIOService() noexcept;

private:

    class EventloopImpl;
    std::unique_ptr<EventloopImpl> _pimpl;
};

}  // End of namespace cadence::async
#endif  // SOLACE_IO_ASYNC_EVENTLOOP_HPP

