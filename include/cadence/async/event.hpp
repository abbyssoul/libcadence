/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadece: Async event
 *	@file		cadence/async/event.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_EVENT_HPP
#define CADENCE_ASYNC_EVENT_HPP

#include "cadence/async/channel.hpp"


namespace cadence { namespace async {

/**
 * An async wrapper for the POSIX eventFd
 */
class Event {
public:

	~Event() = default;

	Event(const Event& rhs) = delete;
    Event& operator= (const Event& rhs) = delete;

    Event(EventLoop& ioContext);

    Event(Event&& rhs);

    Event& operator= (Event&& rhs) noexcept {
        return swap(rhs);
    }

    Event& swap(Event& rhs) noexcept {
        std::swap(_eventFd, rhs._eventFd);

        return *this;
    }


    Solace::Future<void> asyncWait();

    void notify();

private:

    asio::posix::stream_descriptor _eventFd;
    Solace::uint64 _readBuffer;

};

inline void swap(Event& lhs, Event& rhs) noexcept {
    lhs.swap(rhs);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_EVENT_HPP
