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

#include "asio_helper.hpp"


using namespace Solace;
using namespace cadence::async;


class SignalSet::SignalSetImpl {
public:

    SignalSetImpl(void* ioservice, std::initializer_list<int> signal) :
        _signals(*static_cast<asio::io_service*>(ioservice))
    {
        for (auto i : signal) {
            _signals.add(i);
        }
    }


    Future<int> asyncWait() {
        Promise<int> promise;
        auto f = promise.getFuture();

        _signals.async_wait([pm = std::move(promise)] (const asio::error_code& error, int signalNumber) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                pm.setValue(signalNumber);
            }
        });

        return f;
    }


private:
    asio::signal_set _signals;
};


SignalSet::~SignalSet()
{
}


SignalSet::SignalSet(EventLoop& ioContext, std::initializer_list<int> signal) :
    _pimpl(std::make_unique<SignalSetImpl>(ioContext.getIOService(), signal))
{
}


SignalSet::SignalSet(SignalSet&& rhs) :
    _pimpl(std::move(rhs._pimpl))
{
}


SignalSet& SignalSet::swap(SignalSet& rhs) noexcept {
    using std::swap;
    swap(_pimpl, rhs._pimpl);

    return rhs;
}

Future<int> SignalSet::asyncWait() {
    return _pimpl->asyncWait();
}
