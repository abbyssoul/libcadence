/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Async UDP socket
 *	@file		cadence/asyncClient.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNCCLIENT_HPP
#define CADENCE_ASYNCCLIENT_HPP

#include "async/eventloop.hpp"
#include "async/streamsocket.hpp"
#include "protocols/9p2000x.hpp"

#include "networkEndpoint.hpp"


namespace cadence {


class AsyncClient {
public:
    typedef P9Protocol::Tag TransactionId;

public:

    AsyncClient(async::StreamSocket* socket, Solace::MemoryManager& mem);

    AsyncClient(AsyncClient&& rhs) noexcept;

    AsyncClient& operator= (AsyncClient&& rhs) noexcept {
        return swap(rhs);
    }

    AsyncClient& swap(AsyncClient& rhs) noexcept {
        using std::swap;
        swap(_socket, rhs._socket);
        swap(_resourceProtocol, rhs._resourceProtocol);
        swap(_authFid, rhs._authFid);
        swap(_rootFid, rhs._rootFid);
        swap(_transactionPool, rhs._transactionPool);
        swap(_fidMap, rhs._fidMap);

        return *this;
    }


    /**
     * Connect to the resource server.
     * @param endpoint Endpoint to connect to the server.
     * @return Future of connection procedure.
     */
    Solace::Future<void>
    connect(const NetworkEndpoint& endpoint);


    /**
     * Authorise to the specific resource with provided credentials.
     * @param resource Resource to request authorisation to.
     * @param cred Creadential to assess resource access authorisation.
     * @return Future of the authorisation procedure.
     */
    Solace::Future<void>
    auth(const Solace::String& resource, const Solace::String& cred);

    /**
     * Read data given resource / key.
     * @param path A resource name / key to read data from.
     * @return Future data if the read succeed, an error otherwise.
     */
    Solace::Future<Solace::MemoryView>
    read(const Solace::Path& path);

    /**
     * Write given data under the specified path / key.
     * @param path Path / key to store the data under.
     * @param data Data to store.
     * @return Future to indicate successful operation or an error otherwise.
     */
    Solace::Future<void>
    write(const Solace::Path& path, const Solace::ImmutableMemoryView& data);

    /**
     * List resources under the given path / key.
     * Only valid if the path is a directory.
     * @param path Path / key of the directory to list resources under.
     * @return Future of resources under the path or failure otherwise.
     */
    Solace::Future<Solace::Array<Solace::Path>>
    list(const Solace::Path& path);

protected:

    P9Protocol::Fid allocateFid();
    void releaseFid(P9Protocol::Fid fid);

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

    Solace::Future<P9Protocol::Response>
    sendRequest(Transaction& tx);

    Solace::Future<void>
    doAuthDance(const P9Protocol::Qid& authQid,
                const Solace::String& userName,
                const Solace::String& rootName);

    Solace::Future<void>
    doAttachment(const Solace::String& userName, const Solace::String& rootName);


private:


    class TransactionPool {
    public:
        TransactionPool(size_t size, Solace::MemoryManager& memManger);
        Transaction& lookup(TransactionId id);
        Transaction& allocateTransaction();
        void releaseTransaction(TransactionId tx);

    private:
        std::vector<Transaction>  _transactions;
    };

    async::StreamSocket*                _socket;                //!< Communication socket

    std::unique_ptr<P9Protocol>   _resourceProtocol;  //!< Communication protocol to create messages and parse responses
    P9Protocol::Fid          _authFid;           //!< Authentication token
    P9Protocol::Fid          _rootFid;           //!< Root of the tree we are attached to

    TransactionPool _transactionPool;
    std::vector<bool>   _fidMap;
};

}  // End of namespace cadence
#endif  // CADENCE_ASYNCCLIENT_HPP
