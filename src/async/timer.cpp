/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/timer.cpp
 *******************************************************************************/
#include "cadence/async/timer.hpp"

#include "asio_helper.hpp"
#include <asio/steady_timer.hpp>


using namespace Solace;
using namespace cadence::async;


class Timer::TimerImpl {
public:

    TimerImpl(asio::io_context& ioservice) :
        _timer(ioservice)
    {}

    TimerImpl(asio::io_context& ioservice, duration_type duration) :
        _timer(ioservice, duration)
    {}


    void setTimeout(duration_type timeoutDuration) {
        _timer.expires_after(timeoutDuration);
    }

    time_type getTimeout() const {
        return _timer.expiry();
    }

    void cancel() {
        _timer.cancel();
    }

    Future<int64> asyncWait() {
        Promise<int64> promise;
        auto f = promise.getFuture();

        _timer.async_wait([pm = std::move(promise)](asio::error_code const& error) mutable {
            if (error) {
                pm.setError(fromAsioError(error, "asyncWait"));
            } else {
                pm.setValue(1);
            }
        });

        return f;
    }

private:
//    asio::deadline_timer _timer;
    asio::steady_timer _timer;
};



Timer::~Timer() = default;


Timer::Timer(EventLoop& ioContext) :
    _pimpl(std::make_unique<TimerImpl>(asAsioService(ioContext.getIOService())))
{
}

Timer::Timer(EventLoop& ioContext, duration_type durationFromNow) :
    _pimpl(std::make_unique<TimerImpl>(asAsioService(ioContext.getIOService()), durationFromNow))
{
}


Timer::Timer(Timer&& rhs) noexcept
    : _pimpl(std::move(rhs._pimpl))
{
}

Timer& Timer::swap(Timer& rhs) noexcept {
    using std::swap;

    swap(_pimpl, rhs._pimpl);

    return *this;
}


Timer& Timer::setTimeout(duration_type timeoutDuration) {
    _pimpl->setTimeout(timeoutDuration);

    return (*this);
}


Timer::time_type Timer::getTimeout() const {
    return _pimpl->getTimeout();
}



Timer& Timer::cancel() {
    _pimpl->cancel();

    return (*this);
}


Future<int64> Timer::asyncWait() {
    return _pimpl->asyncWait();
}
