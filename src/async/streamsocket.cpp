/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/streamsocket.cpp
 *******************************************************************************/
#include "cadence/async/streamsocket.hpp"
#include "streamsocket_impl.hpp"


using namespace Solace;
using namespace cadence;
using namespace cadence::async;


StreamSocket::StreamSocketImpl::~StreamSocketImpl() = default;


StreamSocket::~StreamSocket() = default;


StreamSocket::StreamSocket(EventLoop& ioContext, std::unique_ptr<StreamSocketImpl> impl) :
    Channel(ioContext),
    _pimpl(std::move(impl))
{ }


Future<void>
StreamSocket::asyncRead(ByteWriter& dest, size_type bytesToRead) {
    return _pimpl->asyncRead(dest, bytesToRead);
}

Future<void>
StreamSocket::asyncWrite(ByteReader& src, size_type bytesToWrite) {
    return _pimpl->asyncWrite(src, bytesToWrite);
}

Result<void, Error>
StreamSocket::read(ByteWriter& dest, size_type bytesToRead) {
    return _pimpl->read(dest, bytesToRead);
}

Result<void, Error>
StreamSocket::write(ByteReader& src, size_type bytesToWrite) {
    return _pimpl->write(src, bytesToWrite);
}


void StreamSocket::cancel() {
    _pimpl->cancel();
}

void StreamSocket::close() {
    _pimpl->close();
}

bool StreamSocket::isOpen() {
    return _pimpl->isOpen();
}


bool StreamSocket::isClosed() {
    return _pimpl->isClosed();
}

NetworkEndpoint StreamSocket::getLocalEndpoint() const {
    return _pimpl->getLocalEndpoint();
}

NetworkEndpoint StreamSocket::getRemoteEndpoint() const {
    return _pimpl->getRemoteEndpoint();
}

void StreamSocket::shutdown() {
    _pimpl->shutdown();
}


Future<void>
StreamSocket::asyncConnect(const NetworkEndpoint& endpoint) {
    return _pimpl->asyncConnect(endpoint);
}


Result<void, Error>
StreamSocket::connect(const NetworkEndpoint& endpoint) {
    return _pimpl->connect(endpoint);
}
