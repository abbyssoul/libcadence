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


AsyncClient::TransactionalMemoryView::~TransactionalMemoryView() {
    if (_txPool)
        _txPool->releaseTransaction(_tag);
}


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
        Solace::raiseInvalidStateError("Transaction lookup failed - no transaction by given id");

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


AsyncClient::AsyncClient(std::unique_ptr<async::Channel> socket, MemoryManager& mem, uint32 concurrencyHint) :
    _memoryManage(&mem),
    _socket(std::move(socket)),
    _resourceProtocol(),
    _authFid(P9Protocol::NOFID),
    _rootFid(P9Protocol::NOFID),
    _transactionPool(concurrencyHint, mem),
    _fidMap(10)
{
}

AsyncClient::AsyncClient(AsyncClient&& rhs) noexcept :
    _memoryManage(std::move(rhs._memoryManage)),
    _socket(std::move(rhs._socket)),
    _resourceProtocol(std::move(rhs._resourceProtocol)),
    _authFid(std::move(rhs._authFid)),
    _rootFid(std::move(rhs._rootFid)),
    _transactionPool(std::move(rhs._transactionPool)),
    _fidMap(std::move(rhs._fidMap))
{}

/*
Future<void>
AsyncClient::connect(const NetworkEndpoint& endpoint) {
    return _socket->connect(endpoint).then([this]() {
        // Prepare the version request
        auto& tx = _transactionPool.allocateTransaction();

        P9Protocol::RequestBuilder(tx.buffer)
                .version();

        // Write it into the pipe
        return asyncSendRequest(tx)
                .then([this, &tx](P9Protocol::Response&& response) {
                    _transactionPool.releaseTransaction(tx.tag);

                    _resourceProtocol.maxNegotiatedMessageSize(response.version.msize);
                    // TODO(abbyssoul): Check if version is supported:
                    _resourceProtocol.setNegotiatedVersion(response.version.version);

                    // Now we should have the whole message
                    return;  // response.version.version;
                });
    });
}
*/

Result<void, Error>
AsyncClient::beginSession(const String& resource, const String& cred) {
    // Prepare the version request
    auto& tx = _transactionPool.allocateTransaction();

    P9Protocol::RequestBuilder(tx.buffer)
            .version();

    // Write it into the pipe
    return sendRequest(tx)
            .then([this](P9Protocol::Response&& response) {
                _resourceProtocol.maxNegotiatedMessageSize(response.version.msize);
                // TODO(abbyssoul): Check if version is supported:
                _resourceProtocol.setNegotiatedVersion(response.version.version);

                // Now we should have the whole message
                return;  // response.version.version;
            })
            .then([this, &resource, &cred]() {
                return doAuth(resource, cred);
            });
}


Future<void>
AsyncClient::asyncBeginSession(const String& resource, const String& cred) {
    // Prepare the version request
    auto& tx = _transactionPool.allocateTransaction();

    P9Protocol::RequestBuilder(tx.buffer)
            .version();

    // Write it into the pipe
    return asyncSendRequest(tx)
            .then([this, &tx](P9Protocol::Response&& response) {
                _transactionPool.releaseTransaction(tx.tag);

                _resourceProtocol.maxNegotiatedMessageSize(response.version.msize);
                // TODO(abbyssoul): Check if version is supported:
                _resourceProtocol.setNegotiatedVersion(response.version.version);

                // Now we should have the whole message
                return;  // response.version.version;
            })
            .then([this, resource, cred]() {
                return doAsyncAuth(resource, cred);
            });
}


Result<void, Error>
AsyncClient::doAuth(const String& resource, const String& cred) {
    // Prepare the version request
    auto& tx = _transactionPool.allocateTransaction();

    _authFid = allocateFid();
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .auth(_authFid, cred, resource);

    // Write it into the pipe
    return sendRequest(tx)
            .then([this, &cred, &resource](P9Protocol::Response&& response) {
                return doAuthDance(response.auth.qid, cred, resource);
            })
            .orElse([this](Error&&) -> Result<void, Error> {
                releaseFid(_authFid);
                _authFid = P9Protocol::NOFID;

                return Ok();  // It's fine - probably not a real error. Just no authentication required!
            })
            .then([&cred, &resource, this]() {
                return doAttachment(cred, resource);
            });
}


Future<void>
AsyncClient::doAsyncAuth(const String& resource, const String& cred) {
    // Prepare the version request
    auto& tx = _transactionPool.allocateTransaction();

    _authFid = allocateFid();
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .auth(_authFid, cred, resource);

    // Write it into the pipe
    return asyncSendRequest(tx)
            .then([cred, resource, &tx, this](P9Protocol::Response&& response) {
                _transactionPool.releaseTransaction(tx.tag);

                return doAsyncAuthDance(response.auth.qid, cred, resource);
            })
            .onError([this](Error&&) -> Result<void, Error> {
                releaseFid(_authFid);
                _authFid = P9Protocol::NOFID;

                return Ok();  // It's fine - probably not a real error. Just no authentication required!
            })
            .then([cred, resource, this]() {
                return doAsyncAttachment(cred, resource);
            });
}


Result<void, Error>
AsyncClient::doAuthDance(const P9Protocol::Qid& /*authQid*/, const String& userName, const String& /*rootName*/) {
    auto& tx = _transactionPool.allocateTransaction();

    String token = userName;  // FIXME: Do a better auth
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .write(_authFid, 0, token.view());

    // Write it into the pipe
    return sendRequest(tx)
            .then([this, &tx](P9Protocol::Response&& /*response*/) {
                // Probably should examine root Qid too.
                return Ok();
            });
}

Future<void>
AsyncClient::doAsyncAuthDance(const P9Protocol::Qid& /*authQid*/, const String& userName, const String& /*rootName*/) {
    auto& tx = _transactionPool.allocateTransaction();

    String token = userName;  // FIXME: Do a better auth
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .write(_authFid, 0, token.view());

    // Write it into the pipe
    return asyncSendRequest(tx)
            .then([this, &tx](P9Protocol::Response&& /*response*/) -> Result<void, Error> {
                _transactionPool.releaseTransaction(tx.tag);

                // Probably should examine root Qid too.
                return Ok();
            });
}


Result<void, Error>
AsyncClient::doAttachment(const String& userName, const String& rootName) {
    auto& tx = _transactionPool.allocateTransaction();

    _rootFid = allocateFid();
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .attach(_rootFid, _authFid,
                    userName, rootName);

    // Write it into the pipe
    return sendRequest(tx)
            .then([&tx, this](P9Protocol::Response&& /*response*/) {
                return Ok();
            });
}


Future<void>
AsyncClient::doAsyncAttachment(const String& userName, const String& rootName) {
    auto& tx = _transactionPool.allocateTransaction();

    _rootFid = allocateFid();
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .attach(_rootFid, _authFid,
                    userName, rootName);

    // Write it into the pipe
    return asyncSendRequest(tx)
            .then([&tx, this](P9Protocol::Response&& /*response*/) -> Result<void, Error> {
                _transactionPool.releaseTransaction(tx.tag);
                // Probably should examine root Qid too.

                return Ok();
            });
}

Future<P9Protocol::size_type>
AsyncClient::open(P9Protocol::Fid fid, P9Protocol::OpenMode mode) {
    auto& tx = _transactionPool.allocateTransaction();
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .open(fid, mode);

    return asyncSendRequest(tx)
            .then([this](P9Protocol::Response&& r) {
                _transactionPool.releaseTransaction(r.tag);

                return r.open.iounit;
            });
}


Future<AsyncClient::TransactionalMemoryView>
AsyncClient::read(P9Protocol::Fid fid, uint64 offset, P9Protocol::size_type iounit) {
    auto& tx = _transactionPool.allocateTransaction();

    if (iounit == 0) {  // Maximum number of butes that can be read in one frame
                        // is the size of the message frame less the message header and data size.
        iounit = _resourceProtocol.maxNegotiatedMessageSize() -
                (_resourceProtocol.headerSize() + sizeof(P9Protocol::size_type));
    }

    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .read(fid, offset, iounit);

    return asyncSendRequest(tx)
            .then([this] (P9Protocol::Response&& response) {
                return TransactionalMemoryView(response.tag,
                                               std::move(response.read.data),
                                               &_transactionPool);
            });
}


Future<P9Protocol::size_type>
AsyncClient::write(P9Protocol::Fid fid, const Solace::ImmutableMemoryView& data) {
    auto& tx = _transactionPool.allocateTransaction();

    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .write(fid, 0, data);

    return asyncSendRequest(tx)
            .then([this] (P9Protocol::Response&& response) {
                _transactionPool.releaseTransaction(response.tag);

                return response.write.count;
            });
}

Future<void>
AsyncClient::clunk(P9Protocol::Fid fid) {
    auto& tx = _transactionPool.allocateTransaction();

    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .clunk(fid);

    releaseFid(fid);

    return asyncSendRequest(tx)
            .then([this](P9Protocol::Response&& resp) {
                _transactionPool.releaseTransaction(resp.tag);
            });
}


Future<AsyncClient::TransactionalMemoryView>
AsyncClient::read(const Path& path) {
    // Prepare the version request
    auto& tx = _transactionPool.allocateTransaction();

    const auto fid = allocateFid();
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .walk(_rootFid, fid, path);

    // Write it into the pipe
    return asyncSendRequest(tx)
            .then([fid, this] (P9Protocol::Response&& r) {
                _transactionPool.releaseTransaction(r.tag);

                return open(fid, P9Protocol::OpenMode::READ);
            }).then([fid, this] (P9Protocol::size_type iounit) {
                return read(fid, 0, iounit);
            })
            .then([this, fid] (TransactionalMemoryView&& txData) -> Result<TransactionalMemoryView, Error> {
                clunk(fid);

                return Ok(std::move(txData));
            });
}


Future<void>
AsyncClient::write(const Path& path, const Solace::ImmutableMemoryView& data) {
    // Prepare the version request
    auto& tx = _transactionPool.allocateTransaction();

    const auto fid = allocateFid();
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .walk(_rootFid, fid, path);

    // Write it into the pipe
    return asyncSendRequest(tx)
            .then([fid, this] (P9Protocol::Response&& r) {
                _transactionPool.releaseTransaction(r.tag);

                return open(fid, P9Protocol::OpenMode::WRITE);
            }).then([fid, &data, this] (P9Protocol::size_type ) {
                return write(fid, data);
            })
            .then([this, fid] (P9Protocol::size_type ) {
                clunk(fid);
            });
}


Future<Array<Path>>
AsyncClient::readDir(P9Protocol::Fid fid, uint64 offset, P9Protocol::size_type iounit, std::vector<Path>&& lst) {
    return read(fid, offset, iounit)
            .then([this, fid, iounit, offset, l = std::move(lst)](TransactionalMemoryView&& txData) mutable {
                const auto dataSize = txData.data.size();
                if (dataSize > 0) {
                    ReadBuffer b(std::move(txData.data));
                    P9Protocol::Decoder decoder(b);

                    while (b.hasRemaining()) {
                        P9Protocol::Stat stat;
                        decoder.read(&stat);

                        l.emplace_back(stat.name);
                    }

                    const auto newOffset = offset + dataSize;
                    return readDir(fid, newOffset, iounit, std::move(l));
                } else {
                    return makeFuture(Array<Path>(std::move(l)));
                }
            });
}


Future<Array<Path>>
AsyncClient::list(const Path& path) {
    // Prepare the version request
    auto& tx = _transactionPool.allocateTransaction();

    const auto fid = allocateFid();
    P9Protocol::RequestBuilder(tx.buffer)
            .tag(tx.tag)
            .walk(_rootFid, fid, path);

    // Write it into the pipe
    return asyncSendRequest(tx)
            .then([fid, this] (P9Protocol::Response&& r) {
                _transactionPool.releaseTransaction(r.tag);

                return open(fid, P9Protocol::OpenMode::READ);
            }).then([fid, this] (P9Protocol::size_type iounit) {
                std::vector<Path> lst;

                return readDir(fid, 0, iounit, std::move(lst));
            })
            .then([this, fid] (Array<Path>&& list) -> Result<Array<Path>, Error> {
                clunk(fid);

                return Ok(std::move(list));
            });
}


Future<P9Protocol::Response>
AsyncClient::asyncSendRequest(Transaction& tx) {
    return _socket->asyncWrite(tx.buffer.flip())
                .then([&tx, this] () {
                    // When that's done - expect a response of a header size:
                    tx.buffer.clear();

                    // FIXME: Support out of order responses!
                    return _socket->asyncRead(tx.buffer, _resourceProtocol.headerSize())
                            .then([&tx, this]() {
                                return _resourceProtocol.parseMessageHeader(tx.buffer.flip());
                            });
                })
                .onError([&tx, this](Error&& e) -> Result<P9Protocol::MessageHeader, Error> {
                    _transactionPool.releaseTransaction(tx.tag);

                    return Err(std::move(e));
                })
                .then([this] (P9Protocol::MessageHeader&& header) {
                    auto& msgTx = _transactionPool.lookup(header.tag);

                    // From the header size let's deduce what is expected message size and read it
                    const auto payloadSize = header.size - _resourceProtocol.headerSize();
                    return _socket->asyncRead(msgTx.buffer.clear(), payloadSize)
                            .then([msgHeader = std::move(header), this]() {
                                auto& msg2Tx = _transactionPool.lookup(msgHeader.tag);

                                return _resourceProtocol.parseResponse(msgHeader, msg2Tx.buffer.flip());
                            })
                            .onError([txId = header.tag, this](Error&& e) -> Result<P9Protocol::Response, Error> {
                                _transactionPool.releaseTransaction(txId);

                                return Err(std::move(e));
                            });
                });
}


Result<P9Protocol::Response, Error>
AsyncClient::sendRequest(Transaction& tx) {

    auto result = _socket->write(tx.buffer.flip())
            .then([this, &tx]() {
                // When we done sending data, expect a response of at least of a header size:
                _socket->read(tx.buffer.clear(), _resourceProtocol.headerSize());
            })
            .then([this, &tx]() {  // We better parse that message header to figure out the size of the whole message
                return _resourceProtocol.parseMessageHeader(tx.buffer.flip());
            })
            .then([this, &tx] (P9Protocol::MessageHeader&& header){
                const auto payloadSize = header.size - _resourceProtocol.headerSize();

                // Read the message payload
                return _socket->read(tx.buffer.clear(), payloadSize)
                        .then([this, &tx, &header]() {
                            return _resourceProtocol.parseResponse(header, tx.buffer.flip());
                        });
            });

    _transactionPool.releaseTransaction(tx.tag);

    return result;
}
