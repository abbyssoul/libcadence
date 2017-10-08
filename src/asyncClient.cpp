/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/

#include "cadence/asyncClient.hpp"


using namespace Solace;
using namespace cadence;


AsyncClient::TransactionPool::TransactionPool(size_t size, MemoryManager& memManger) {
    for (size_t i = 0; i < size; ++i) {
        _transactions.emplace_back(i, ByteBuffer(memManger.create(P9Protocol::MAX_MESSAGE_SIZE)));
    }
}

AsyncClient::Transaction&
AsyncClient::TransactionPool::lookup(TransactionId tag) {
    if (tag == P9Protocol::NO_TAG)
        return _transactions.front();

    auto i = std::find_if(_transactions.begin(), _transactions.end(),
                        [tag](const auto& tx) { return tx.tag == tag; });

    if (i == _transactions.end())
        Solace::raiseInvalidStateError();

    return *i;
}

AsyncClient::Transaction&
AsyncClient::TransactionPool::allocateTransaction() {
    auto i = std::find_if(_transactions.begin(), _transactions.end(),
                        [](const auto& tx) { return !tx.awaited; });

    if (i == _transactions.end())
        Solace::raiseInvalidStateError("Can't allocate any more transactions");

    i->awaited = true;
    return *i;
}


void
AsyncClient::TransactionPool::releaseTransaction(TransactionId tag) {
    auto& tx = lookup(tag);

    if (!tx.awaited)
        Solace::raiseInvalidStateError("Logic error: releasing transtion that on one awaits");

    tx.buffer.clear();
    tx.awaited = false;
}



Future<P9Protocol::Response>
AsyncClient::sendRequest(Transaction& tx) {
    return _socket->asyncWrite(tx.buffer.flip())
                .then([&tx, this] () {
                    // When that's done - expect a response of a header size:
                    tx.buffer.clear();

                    // FIXME: Support out of order responses!
                    return _socket->asyncRead(tx.buffer, _resourceProtocol->headerSize())
                            .then([&tx, this]() {
                                return _resourceProtocol->parseMessageHeader(tx.buffer.flip());
                            });
                })
                .onError([&tx, this](Solace::Error&& e) -> Result<P9Protocol::MessageHeader, Solace::Error> {
                    _transactionPool.releaseTransaction(tx.tag);

                    return Err(std::move(e));
                })
                .then([this] (P9Protocol::MessageHeader&& header) {
                    auto& msgTx = _transactionPool.lookup(header.tag);

                    // From the header size let's deduce what is expected message size and read it
                    const auto payloadSize = header.size - _resourceProtocol->headerSize();
                    return _socket->asyncRead(msgTx.buffer.clear(), payloadSize)
                            .then([msgHeader = std::move(header), this]() {
                                auto& msg2Tx = _transactionPool.lookup(msgHeader.tag);

                                return _resourceProtocol->parseResponse(msgHeader, msg2Tx.buffer.flip());
                            })
                            .onError([txId = header.tag, this](Solace::Error&& e)
                                     -> Result<P9Protocol::Response, Solace::Error> {
                                _transactionPool.releaseTransaction(txId);

                                return Err(std::move(e));
                            });
                });
}


P9Protocol::Fid
AsyncClient::allocateFid() {
    auto i = std::find(_fidMap.begin(), _fidMap.end(), false);
    if (i == _fidMap.end()) {
        _fidMap.push_back(true);
        i = _fidMap.end()--;
    } else {
        *i = true;
    }


    return std::distance(_fidMap.begin(), i);
}

void
AsyncClient::releaseFid(P9Protocol::Fid fid) {
    _fidMap[fid] = false;
}


AsyncClient::AsyncClient(async::StreamSocket* socket, Solace::MemoryManager& mem) :
    _socket(socket),
    _resourceProtocol(std::make_unique<P9Protocol>()),
    _authFid(P9Protocol::NOFID),
    _rootFid(P9Protocol::NOFID),
    _transactionPool(2, mem),
  _fidMap(10)
{
}

AsyncClient::AsyncClient(AsyncClient&& rhs) noexcept :
    _socket(std::move(rhs._socket)),
    _resourceProtocol(std::move(rhs._resourceProtocol)),
    _authFid(std::move(rhs._authFid)),
    _rootFid(std::move(rhs._rootFid)),
    _transactionPool(std::move(rhs._transactionPool))
{}


Future<void>
AsyncClient::connect(const NetworkEndpoint& endpoint) {
    return _socket->asyncConnect(endpoint).then([this]() {
        // Prepare the version request
        auto& tx = _transactionPool.allocateTransaction();

        P9Protocol::RequestBuilder(tx.buffer)
                .version();

        // Write it into the pipe
        return sendRequest(tx)
                .then([this, &tx](P9Protocol::Response&& response) {
                    _transactionPool.releaseTransaction(tx.tag);

                    _resourceProtocol->maxNegotiatedMessageSize(response.version.msize);
                    // TODO(abbyssoul): Check if version is supported:
                    _resourceProtocol->setNegotiatedVersion(response.version.version);

                    // Now we should have the whole message
                    return;  // response.version.version;
                });
    });
}


Future<void>
AsyncClient::auth(const String& resource, const String& cred) {
    // Prepare the version request
    auto& tx = _transactionPool.allocateTransaction();

    _authFid = allocateFid();
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .auth(_authFid, cred, resource);

    // Write it into the pipe
    return sendRequest(tx)
            .then([cred, resource, &tx, this](P9Protocol::Response&& response) {
                _transactionPool.releaseTransaction(tx.tag);

                return doAuthDance(response.auth.qid, cred, resource);
            })
            .onError([](Error&&) -> Result<void, Error> {
                return Ok();  // It's fine - not a real error. Just no authentication required!
            })
            .then([cred, resource, this]() {
                return doAttachment(cred, resource);
            });
}


Future<void>
AsyncClient::doAuthDance(const P9Protocol::Qid& authQid, const String& userName, const String& rootName) {
    auto& tx = _transactionPool.allocateTransaction();

    _authFid = allocateFid();
    String token = userName;  // FIXME: Do a better auth
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .write(_authFid, 0, token.view());

    // Write it into the pipe
    return sendRequest(tx)
            .then([this, &tx](P9Protocol::Response&& /*response*/) -> Result<void, Solace::Error> {
                _transactionPool.releaseTransaction(tx.tag);

                // Probably should examine root Qid too.
                return Ok();
            });
}


Future<void>
AsyncClient::doAttachment(const String& userName, const String& rootName) {
    auto& tx = _transactionPool.allocateTransaction();

    _rootFid = allocateFid();
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .attach(_rootFid, _authFid,
                    userName, rootName);

    // Write it into the pipe
    return sendRequest(tx)
            .then([&tx, this](P9Protocol::Response&& /*response*/) -> Result<void, Solace::Error> {
                _transactionPool.releaseTransaction(tx.tag);
                // Probably should examine root Qid too.

                return Ok();
            });
}


Future<MemoryView>
AsyncClient::read(const Path& path) {
    // Prepare the version request
    auto& tx = _transactionPool.allocateTransaction();

    const auto fid = allocateFid();
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .walk(_rootFid, fid, path);

    // Write it into the pipe
    return sendRequest(tx)
            .then([fid, &tx, this] (P9Protocol::Response&& ) {
                _transactionPool.releaseTransaction(tx.tag);

                auto& tx2 = _transactionPool.allocateTransaction();
                P9Protocol::RequestBuilder(tx.buffer)
                        .tag(tx2.tag)
                        .open(fid, 1);

                return sendRequest(tx2)
                        .then([&tx2, this](P9Protocol::Response&& ) {
                            _transactionPool.releaseTransaction(tx2.tag);
                        });
            }).then([fid, this] () {
                auto& readTx = _transactionPool.allocateTransaction();

                P9Protocol::RequestBuilder(readTx.buffer)
                        .tag(readTx.tag)
                        .read(fid, 0, _resourceProtocol->maxNegotiatedMessageSize());

                return sendRequest(readTx);
            }).then([this] (P9Protocol::Response&& response) {
                return response.read.data.viewShallow();
            })
            .then([this, fid] (MemoryView&& data) -> Result<MemoryView, Error> {
                auto& clunkTx = _transactionPool.allocateTransaction();

                P9Protocol::RequestBuilder(clunkTx.buffer)
                        .tag(clunkTx.tag)
                        .clunk(fid);

                releaseFid(fid);

                sendRequest(clunkTx).then([&clunkTx, this](P9Protocol::Response&&) {
                    _transactionPool.releaseTransaction(clunkTx.tag);
                });

                return Ok(std::move(data));
            });
}
