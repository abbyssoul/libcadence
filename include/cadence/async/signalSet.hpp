/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadece: Async System Signal event
 *	@file		cadence/async/event.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_SIGNALSET_HPP
#define CADENCE_ASYNC_SIGNALSET_HPP

#include "cadence/async/channel.hpp"


namespace cadence { namespace async {

/**
 * An async interface for POSIX signals
 */
class SignalSet {
public:

	~SignalSet() = default;

    SignalSet(const SignalSet& rhs) = delete;

    SignalSet& operator= (const SignalSet& rhs) = delete;

    SignalSet(EventLoop& ioContext, std::initializer_list<int> signal);

    /* FIXME: Need to patch asio::signal_set to be movable
    SignalSet(SignalSet&& rhs);

    SignalSet& operator= (SignalSet&& rhs) noexcept {
        return swap(rhs);
    }

    SignalSet& swap(SignalSet& rhs) noexcept {
        using std::swap;

        return rhs;
    }
    */

    Solace::Future<int> asyncWait();

private:
    asio::signal_set _signals;
};


/* FIXME: Need to patch asio::signal_set to be movable
inline void swap(SignalSet& lhs, SignalSet& rhs) noexcept {
    lhs.swap(rhs);
}
*/

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_SIGNALSET_HPP
