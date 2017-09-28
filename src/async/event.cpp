/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/event.cpp
 *******************************************************************************/
#include "cadence/async/event.hpp"

#include <solace/exception.hpp>

#include <sys/eventfd.h>

#include "asio.hpp"


using namespace Solace;
using namespace cadence::async;


Event::Event(EventLoop& ioContext) :
    _eventFd(ioContext.getIOService(), eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC))
{
}


Event::Event(Event&& rhs) :
    _eventFd(std::move(rhs._eventFd))
{
}


void Event::notify() {
    const auto result = eventfd_write(_eventFd.native_handle(), 1);

    if (result < 0) {
        raise<IOException>(errno);
    }
}


Future<void> Event::asyncWait() {
    Promise<void> promise;
    auto f = promise.getFuture();

    _eventFd.async_read_some(asio::buffer(&_readBuffer, sizeof(_readBuffer)),
        [pm = std::move(promise)] (const asio::error_code& error, std::size_t) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
			pm.setValue();
        }
	});

    return f;
}

