/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Channel interface for async events
 *	@file		cadence/async/channel.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_CHANNEL_HPP
#define CADENCE_ASYNC_CHANNEL_HPP


#include "cadence/async/eventloop.hpp"


namespace cadence { namespace async {


/**
 * Channel is the base of the io objects.
 * Subclasses of this class provide access to the real IO subsystems
 */
class Channel {
public:
    typedef EventLoop::size_type size_type;

public:

    virtual ~Channel() = default;

    Channel(EventLoop& ioContext) :
        _ioContext(&ioContext)
    {
    }

    Channel(const Channel&) = delete;

    Channel(Channel&& rhs) :
        _ioContext(&rhs.getIOContext())
    {}

    Channel& operator= (Channel&& rhs) noexcept {
        return swap(rhs);
    }

    Channel& swap(Channel& rhs) noexcept {
        std::swap(_ioContext, rhs._ioContext);

        return *this;
    }

    EventLoop& getIOContext() noexcept {
        return *_ioContext;
    }

    const EventLoop& getIOContext() const noexcept {
        return *_ioContext;
    }

private:

    EventLoop*   _ioContext;
};


inline void swap(Channel& lhs, Channel& rhs) noexcept {
    lhs.swap(rhs);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_CHANNEL_HPP
