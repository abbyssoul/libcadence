/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/acceptor.cpp
 *******************************************************************************/
#include "cadence/async/acceptor.hpp"


using namespace Solace;
using namespace cadence;
using namespace cadence::async;



std::unique_ptr<Acceptor::AcceptorImpl> createTCPAcceptor(EventLoop& loop);
std::unique_ptr<Acceptor::AcceptorImpl> createUnixDomainAcceptor(EventLoop& loop);


Acceptor::Acceptor(EventLoop& ioContext)
    : _loop(ioContext)
    , _pimpl()
{
}


Result<void, Error>
Acceptor::open(NetworkEndpoint const& endpoint) {

    if (_pimpl)
        return Err(Error("Already opened"));

    _pimpl = std::visit([this](auto&& e) {
        using T = std::decay_t<decltype(e)>;

        if constexpr (std::is_same_v<T, UnixEndpoint>)  {
            return createUnixDomainAcceptor(_loop);
        } else if constexpr (std::is_same_v<T, IPEndpoint>)  {
            return createTCPAcceptor(_loop);
        }

    }, endpoint);

    return _pimpl->open(endpoint);
}


bool Acceptor::nonBlocking() {
    return _pimpl->nonBlocking();
}

bool Acceptor::nativeNonBlocking() {
    return _pimpl->nativeNonBlocking();
}

void Acceptor::nativeNonBlocking(bool mode) {
    _pimpl->nativeNonBlocking(mode);
}


void Acceptor::cancel() {
    _pimpl->cancel();
}

void Acceptor::close() {
    _pimpl->close();
}

bool Acceptor::isOpen() {
    return _pimpl->isOpen();
}

bool Acceptor::isClosed() {
    return _pimpl->isClosed();
}

NetworkEndpoint Acceptor::getLocalEndpoint() const {
    return _pimpl->getLocalEndpoint();
}


Result<StreamSocket, Error>
Acceptor::accept() {
    return _pimpl->accept();
}

Future<StreamSocket>
Acceptor::asyncAccept() {
    return _pimpl->asyncAccept();
}

