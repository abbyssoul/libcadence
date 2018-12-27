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
#include <asio/deadline_timer.hpp>


using namespace Solace;
using namespace cadence::async;


class Timer::TimerImpl {
public:
    TimerImpl(void* ioservice) :
        _timer(asAsioService(ioservice))
    {}

    TimerImpl(void* ioservice, duration_type duration) :
        _timer(asAsioService(ioservice), boost::posix_time::milliseconds(duration.count()))
    {}


    void setTimeout(duration_type timeoutDuration) {
        _timer.expires_from_now(boost::posix_time::milliseconds(timeoutDuration.count()));
    }


    duration_type getTimeout() const {
        const auto f = _timer.expires_from_now().fractional_seconds();

        return duration_type(f);
    }

    void cancel() {
        _timer.cancel();
    }

    Future<int64> asyncWait() {
        Promise<int64> promise;
        auto f = promise.getFuture();

        _timer.async_wait([pm = std::move(promise)](const asio::error_code& error) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                pm.setValue(1);
            }
        });

        return f;
    }

private:
    asio::deadline_timer _timer;
};



Timer::~Timer() = default;


Timer::Timer(EventLoop& ioContext) :
    _pimpl(std::make_unique<TimerImpl>(ioContext.getIOService()))
{
}

Timer::Timer(EventLoop& ioContext, duration_type durationFromNow) :
    _pimpl(std::make_unique<TimerImpl>(ioContext.getIOService(), durationFromNow))
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


Timer::duration_type Timer::getTimeout() const {
    return _pimpl->getTimeout();
}



Timer& Timer::cancel() {
    _pimpl->cancel();

	return (*this);
}


Future<int64> Timer::asyncWait() {
    return _pimpl->asyncWait();
}
