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
#include <cadence/async/timer.hpp>

#include <sys/timerfd.h>

#include "asio.hpp"


using namespace Solace;
using namespace cadence::async;


Timer::Timer(EventLoop& ioContext) :
    _timer(ioContext.getIOService())
{
}


Timer::Timer(EventLoop& ioContext, time_type initialTimeout) :
    _timer(ioContext.getIOService(), initialTimeout)
{
}


Timer::Timer(EventLoop& ioContext, duration_type durationFromNow) :
    _timer(ioContext.getIOService(), durationFromNow)
{
}


Timer::Timer(Timer&& rhs) :
    _timer(std::move(rhs._timer))
{
}


Timer& Timer::setTimeout(duration_type timeoutDuration) {
    _timer.expires_from_now(timeoutDuration);

    return (*this);
}


Timer::duration_type Timer::getTimeout() const {
    return _timer.expires_from_now();
}


Timer& Timer::cancel() {
    _timer.cancel();

	return (*this);
}


Future<int64>
Timer::asyncWait() {
    Promise<int64> promise;
    auto f = promise.getFuture();

    _timer.async_wait([pm = std::move(promise)](const asio::error_code& error) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
            pm.setValue(1);
		}
    });

    return f;
}
