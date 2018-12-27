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

#include "asio_helper.hpp"
#include <asio/posix/stream_descriptor.hpp>


using namespace Solace;
using namespace cadence::async;

class Event::EventImpl {
public:
    EventImpl(void* ioservice):
        _eventFd(asAsioService(ioservice), eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC))
    {}

    void notify() {
        const auto result = eventfd_write(_eventFd.native_handle(), 1);

        if (result < 0) {
            raise<IOException>(errno);
        }
    }

    Future<void> asyncWait() {
        Promise<void> promise;
        auto f = promise.getFuture();

        _eventFd.async_read_some(asio::buffer(&_readBuffer, sizeof(_readBuffer)),
            [pm = std::move(promise)] (const asio::error_code& error, std::size_t) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                pm.setValue();
            }
        });

        return f;
    }

private:
    asio::posix::stream_descriptor _eventFd;
    Solace::uint64 _readBuffer;
};


Event::~Event() = default;

Event::Event(EventLoop& ioContext) :
    _pimpl(std::make_unique<EventImpl>(ioContext.getIOService()))
{
}


Event::Event(Event&& rhs) :
    _pimpl(std::move(rhs._pimpl))
{
}

Event& Event::swap(Event& rhs) noexcept {
    std::swap(_pimpl, rhs._pimpl);

    return *this;
}


void Event::notify() {
    _pimpl->notify();
}


Future<void> Event::asyncWait() {
    return _pimpl->asyncWait();
}

