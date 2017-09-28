/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/_signalset.cpp
 *******************************************************************************/
#include <cadence/async/signalSet.hpp>

#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>

#include "asio.hpp"


using namespace Solace;
using namespace cadence::async;



SignalSet::SignalSet(EventLoop& ioContext, std::initializer_list<int> signal) :
    _signals(ioContext.getIOService())
{
    for (auto i : signal) {
		_signals.add(i);
	}
}


/* FIXME: Need to patch asio::signal_set to be movable
SignalSet::SignalSet(SignalSet&& rhs) :
    _signals(std::move(rhs))
{
}
*/

Future<int> SignalSet::asyncWait() {
    Promise<int> promise;
    auto f = promise.getFuture();

    _signals.async_wait([pm = std::move(promise)] (const asio::error_code& error, int signalNumber) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
            pm.setValue(signalNumber);
        }
	});

	return f;
}
