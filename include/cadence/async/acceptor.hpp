/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Async acceptor interface.
 *	@file		cadence/async/acceptor.hpp
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_ACCEPTOR_HPP
#define CADENCE_ASYNC_ACCEPTOR_HPP

#include "cadence/networkEndpoint.hpp"
#include "cadence/async/streamsocket.hpp"

#include <solace/result.hpp>
#include <solace/future.hpp>


namespace cadence { namespace async {

/**
 * Acceptor class for stream-oriented sockets.
 * The class of NetworkAddress used to open the acceptor determines which actual acceptor will be used.
 */
class Acceptor {
public:

    ~Acceptor() = default;

    Acceptor(EventLoop& loop);

    Acceptor(Acceptor const&) = delete;
    Acceptor& operator= (Acceptor const&) = delete;

    Acceptor(Acceptor&&) = default;
    Acceptor& operator= (Acceptor&&) = default;

    Acceptor& swap(Acceptor& rhs) noexcept {
        using std::swap;
        swap(_pimpl, rhs._pimpl);

        return *this;
    }

    /**
     * Open the acceptor on the given end-point and start listenning for incomming connection requests.
     * @param endpoint Local endpoint to bind to.
     * @return Result of the binding / listenning operation.
     */
    Solace::Result<void, Solace::Error>
    open(NetworkEndpoint const& endpoint);

    /**
     * Determine whether the acceptor is open.
     * @return True is the acceptor is opened and accepts connections.
     */
    bool isOpen();

    /**
     * Close the acceptor. No more connection will be accepted after this operation.
     */
    void close();

    /**
     * Check if acceptor has been closed.
     */
    bool isClosed();

    /**
     * Accept a new connection.
     * The function call will block until a new connection has been accepted successfully or an error occurs.
     * @return A socket object representing the newly accepted connection.
     */
    Solace::Result<StreamSocket, Solace::Error>
    accept();

    /** Start an asynchronous accept.
     * This function is used to asynchronously accept a new connection.
     * @return Future of the newly accepted socket or an error.
     */
    Solace::Future<StreamSocket>
    asyncAccept();

    /**
     * Gets the non-blocking mode of the acceptor.
     * @return True if acceptor is in non blocking mode.
     */
    bool nonBlocking();
    /**
     * Sets the non-blocking mode of the acceptor.
     * @param mode If false, synchronous operations will block until complete.
     */
    void nonBlocking(bool mode);

    /**
     * Gets the non-blocking mode of the native acceptor implementation.
     */
    bool nativeNonBlocking();

    /**
     * Sets the non-blocking mode of the native acceptor implementation.
     * @param mode True for non-bloking.
     */
    void nativeNonBlocking(bool mode);

    /**
     * Cancel all asynchronous operations associated with the acceptor.
     */
    void cancel();

    /**
     * Get the local endpoint of the acceptor socket.
     * @return Local endpoint this acceptor is bound to.
     */
    NetworkEndpoint getLocalEndpoint() const;

public:

    class AcceptorImpl {
    public:

        virtual ~AcceptorImpl() = default;

        /** @see Acceptor::open */
        virtual Solace::Result<void, Solace::Error>
        open(NetworkEndpoint const& endpoint) = 0;

        /** @see Acceptor::isOpen */
        virtual bool isOpen() = 0;

        /** @see Acceptor::close */
        virtual void close() = 0;

        /** @see Acceptor::isClose */
        virtual bool isClosed() = 0;

        virtual Solace::Result<StreamSocket, Solace::Error>
        accept() = 0;

        virtual Solace::Future<StreamSocket>
        asyncAccept() = 0;

        /** @see Acceptor::nonBlocking */
        virtual bool nonBlocking() = 0;

        /** @see Acceptor::nonBlocking */
        virtual void nonBlocking(bool mode) = 0;

        /** @see Acceptor::nativeNonBlocking */
        virtual bool nativeNonBlocking() = 0;

        /** @see Acceptor::nativeNonBlocking */
        virtual void nativeNonBlocking(bool mode) = 0;

        /** @see Acceptor::cancel */
        virtual void cancel() = 0;

        /** @see Acceptor::getLocalEndpoint */
        virtual NetworkEndpoint getLocalEndpoint() const = 0;
    };

private:

    std::reference_wrapper<EventLoop>   _loop;
    std::unique_ptr<AcceptorImpl>       _pimpl;
};

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_ACCEPTOR_HPP
