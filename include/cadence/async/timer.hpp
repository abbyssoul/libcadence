/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Async Timer
 *	@file		cadence/async/timer.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_TIMER_HPP
#define CADENCE_ASYNC_TIMER_HPP

#include "cadence/async/channel.hpp"

#include <chrono>


namespace cadence { namespace async {

/**
 * TODO(abbyssoul): document this class
 */
class Timer {
public:
    //!< Absolute point in time
    typedef asio::deadline_timer::time_type time_type;

    //!< Duration of time
    typedef asio::deadline_timer::duration_type duration_type;

public:

    ~Timer() = default;

    Timer(const Timer& rhs) = delete;

    Timer& operator= (const Timer& rhs) = delete;

    Timer(EventLoop& ioContext);

    Timer(EventLoop& ioContext, time_type pointInTime);

    Timer(EventLoop& ioContext, duration_type durationFromNow);

    Timer(Timer&& rhs);

    Timer& operator= (Timer&& rhs) noexcept {
        return swap(rhs);
    }

    Timer& operator> (Timer&& rhs) noexcept {
        return swap(rhs);
    }

    Timer& swap(Timer& rhs) noexcept {
        using std::swap;

        swap(_timer, rhs._timer);

        return *this;
    }


    Solace::Future<Solace::int64> asyncWait();

    Timer& setTimeout(duration_type timeoutDuration);

    duration_type getTimeout() const;

	Timer& cancel();

private:
    asio::deadline_timer _timer;

};

inline void swap(Timer& lhs, Timer& rhs) noexcept {
    lhs.swap(rhs);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_TIMER_HPP
