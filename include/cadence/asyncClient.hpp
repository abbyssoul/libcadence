/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Async Resource server client
 *	@file		cadence/asyncClient.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNCCLIENT_HPP
#define CADENCE_ASYNCCLIENT_HPP

#include "networkEndpoint.hpp"

#include "async/eventloop.hpp"
#include "async/streamsocket.hpp"

#include "protocols/9p2000x.hpp"


namespace cadence {


class AsyncClient {
protected:
    class TransactionPool;

public:
    using TransactionId = P9Protocol::Tag;

    struct TransactionalMemoryView {

        ~TransactionalMemoryView();

        TransactionalMemoryView(TransactionId txId, Solace::ImmutableMemoryView&& mem, TransactionPool* pool) :
            data(std::move(mem)),
            _tag(txId),
            _txPool(pool)
        {}

        TransactionalMemoryView(TransactionalMemoryView&& other) :
            data(std::move(other.data)),
            _tag(std::move(other._tag)),
            _txPool(other._txPool)
        {
            other._txPool = nullptr;
        }

        Solace::ImmutableMemoryView data;

    protected:
        TransactionId       _tag;
        TransactionPool*    _txPool;
    };

public:

    /**
     * Construct async-client with a connect socket and a memory manager.
     * @param socket Already connect stream-oriented socket.
     * @param mem Memory manager used for read buffer allocation.
     * @param concurrencyHint Maximum number of async requests at any given time.
     */
    AsyncClient(std::unique_ptr<async::Channel> socket, Solace::MemoryManager& mem, Solace::uint32 concurrencyHint = 2);

    /// Move constructor
    AsyncClient(AsyncClient&& rhs) noexcept = default;

    AsyncClient& operator= (AsyncClient&& rhs) noexcept {
        return swap(rhs);
    }

    AsyncClient& swap(AsyncClient& rhs) noexcept {
        using std::swap;
        swap(_memoryManage, _memoryManage);
        swap(_socket, rhs._socket);
        swap(_resourceProtocol, rhs._resourceProtocol);
        swap(_authFid, rhs._authFid);
        swap(_rootFid, rhs._rootFid);
        swap(_transactionPool, rhs._transactionPool);
        swap(_fidMap, rhs._fidMap);

        return *this;
    }


    /**
     * Establish a new session with the resource server.
     * @return Result of an established session or an error.
     */
    Solace::Result<void, Solace::Error>
    beginSession(const Solace::String& resource, const Solace::String& cred);

    /**
     * Establish a new session with the resource server.
     * @return Future of an established session, or an error.
     */
    Solace::Future<void>
    beginSessionAsync(const Solace::String& resource, const Solace::String& cred);

    /**
     * Read data given resource / key.
     * @param path A resource name / key to read data from.
     * @return Future data if the read succeed, an error otherwise.
     */
    Solace::Future<TransactionalMemoryView>
    readAsync(const Solace::Path& path);

    /**
     * Write given data under the specified path / key.
     * @param path Path / key to store the data under.
     * @param data Data to store.
     * @return Future to indicate successful operation or an error otherwise.
     */
    Solace::Future<void>
    writeAsync(const Solace::Path& path, const Solace::ImmutableMemoryView& data);

    /**
     * List resources under the given path / key.
     * Only valid if the path is a directory.
     * @param path Path / key of the directory to list resources under.
     * @return Future of resources under the path or failure otherwise.
     */
    Solace::Future<Solace::Array<Solace::Path>>
    listAsync(const Solace::Path& path);

protected:

    struct Transaction {
        const TransactionId tag;
        bool awaited;
        Solace::ByteBuffer buffer;

        Transaction(TransactionId&& _tag,
                    Solace::ByteBuffer&& _buffer) :
            tag(std::move(_tag)),
            awaited(false),
            buffer(std::move(_buffer))
        {}
    };

    class TransactionPool {
    public:
        TransactionPool(size_t size, Solace::MemoryManager& memManger);
        Transaction& lookup(TransactionId id);
        Transaction& allocateTransaction();
        void releaseTransaction(TransactionId tx);

    private:
        std::vector<Transaction>  _transactions;
    };


protected:

    P9Protocol::Fid allocateFid();
    void releaseFid(P9Protocol::Fid fid);


    Solace::Future<P9Protocol::Response>
    asyncSendRequest(Transaction& tx);

    Solace::Result<P9Protocol::Response, Solace::Error>
    sendRequest(Transaction& tx);


    /**
     * Authorise to the specific resource with provided credentials.
     * @param resource Resource to request authorisation to.
     * @param cred Creadential to assess resource access authorisation.
     * @return Future of the authorisation procedure.
     */
    Solace::Result<void, Solace::Error> doAuth(const Solace::String& resource, const Solace::String& cred);

    Solace::Result<void, Solace::Error>
    doAuthDance(const P9Protocol::Qid& authQid,
                const Solace::String& userName,
                const Solace::String& rootName);

    Solace::Result<void, Solace::Error>
    doAttachment(const Solace::String& userName, const Solace::String& rootName);


    /**
     * Authorise to the specific resource with provided credentials.
     * @param resource Resource to request authorisation to.
     * @param cred Creadential to assess resource access authorisation.
     * @return Future of the authorisation procedure.
     */
    Solace::Future<void> doAsyncAuth(const Solace::String& resource, const Solace::String& cred);

    Solace::Future<void>
    doAsyncAuthDance(const P9Protocol::Qid& authQid,
                const Solace::String& userName,
                const Solace::String& rootName);

    Solace::Future<void>
    doAsyncAttachment(const Solace::String& userName, const Solace::String& rootName);

    Solace::Future<P9Protocol::size_type>
    open(P9Protocol::Fid fid, P9Protocol::OpenMode mode);

    Solace::Future<TransactionalMemoryView>
    read(P9Protocol::Fid fid, Solace::uint64 offset, P9Protocol::size_type iounit = 0);

    Solace::Future<P9Protocol::size_type>
    write(P9Protocol::Fid fid, const Solace::ImmutableMemoryView& data);

    Solace::Future<void>
    clunk(P9Protocol::Fid fid);

    Solace::Future<Solace::Array<Solace::Path>>
    readDir(P9Protocol::Fid fid, Solace::uint64 offset, P9Protocol::size_type iounit, std::vector<Solace::Path>&& list);

private:

    Solace::MemoryManager*              _memoryManage;
    std::unique_ptr<async::Channel>     _socket;                //!< Communication socket

    P9Protocol                  _resourceProtocol;  //!< Communication protocol to create messages and parse responses
    P9Protocol::Fid             _authFid;           //!< Authentication token
    P9Protocol::Fid             _rootFid;           //!< Root of the tree we are attached to

    TransactionPool             _transactionPool;
    std::vector<bool>           _fidMap;
};

}  // End of namespace cadence
#endif  // CADENCE_ASYNCCLIENT_HPP
