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
    using time_type = std::chrono::steady_clock::time_point;

    //!< Duration of time
    using duration_type = std::chrono::milliseconds;

public:

    ~Timer();

    Timer(const Timer& rhs) = delete;
    Timer& operator= (const Timer& rhs) = delete;

    Timer(Timer&& rhs) noexcept;

    Timer(EventLoop& ioContext);
    Timer(EventLoop& ioContext, duration_type durationFromNow);


    Timer& operator= (Timer&& rhs) noexcept {
        return swap(rhs);
    }

    Timer& swap(Timer& rhs) noexcept;

    Solace::Future<Solace::int64> asyncWait();

    Timer& setTimeout(duration_type timeoutDuration);

    time_type getTimeout() const;

    Timer& cancel();

private:
    class TimerImpl;
    std::unique_ptr<TimerImpl> _pimpl;
};

inline void swap(Timer& lhs, Timer& rhs) noexcept {
    lhs.swap(rhs);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_TIMER_HPP
