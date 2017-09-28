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
 *	@file		cadence/async/udpsocket.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_UDPSOCKET_HPP
#define CADENCE_ASYNC_UDPSOCKET_HPP

#include "cadence/async/channel.hpp"

#include <solace/string.hpp>


namespace cadence { namespace async {

/**
 * TODO(abbyssoul): document this class
 */
class UdpSocketReceiver {
public:
    typedef Solace::String endpoint_type;

public:

    ~UdpSocketReceiver()= default;

    UdpSocketReceiver(const endpoint_type& receiver, Solace::String serviceName) :
        _receiver(receiver),
        _serviceName(serviceName)
    {
    }

    endpoint_type& getReceiver() {
        return _receiver;
    }

    Solace::String& getServiceName() {
        return _serviceName;
    }

private:
    endpoint_type _receiver;
    Solace::String _serviceName;
};


/**
 * An async wrapper for the POSIX UdpSocket
 */
class UdpSocket {
public:
    typedef Solace::String endpoint_type;

public:

    ~UdpSocket() = default;

    UdpSocket(EventLoop& eventLoop, Solace::uint16 port);

    UdpSocket(const UdpSocket& rhs) = delete;

    UdpSocket& operator= (const UdpSocket& rhs) = delete;

    UdpSocket(EventLoop& ioContext);

    UdpSocket(UdpSocket&& rhs);

    UdpSocket& operator= (UdpSocket&& rhs) noexcept {
        return swap(rhs);
    }

    UdpSocket& swap(UdpSocket& rhs) noexcept {
        using std::swap;
        swap(_socket, rhs._socket);

        return *this;
    }

	/**
     * Post an async read request to read data from this IO object into the given buffer.
     * This method reads the data until the provided destination buffer is full.
     *
     * @param dest The provided destination buffer to read data into.
     * @return A future that will be resolved one the buffer has been filled.
     */
    Solace::Future<void> asyncRead(Solace::ByteBuffer& dest) {
        return asyncRead(dest, dest.remaining());
    }

    /**
     * Post an async read request to read specified amount of data from this IO object into the given buffer.
     *
     * @param dest The provided destination buffer to read data into.
     * @param bytesToRead Amount of data (in bytes) to read from this IO object.
     * @return A future that will be resolved one the scpecified number of bytes has been read.
     *
     * @note If the provided destination buffer is too small to hold requested amount of data - an exception is raised.
     */
    Solace::Future<void> asyncRead(Solace::ByteBuffer& dest, std::size_t bytesToRead);

    /**
     * Post an async write request to write specified amount of data into this IO object.
     * This method writes whole content of the provided buffer into the IO objec.
     *
     * @param src The provided source buffer to read data from.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     */
    Solace::Future<void> asyncWrite(Solace::ByteBuffer& src,
                                    const endpoint_type& receiver,
                                    const Solace::String& serviceName) {
        return asyncWrite(src, receiver, serviceName, src.remaining());
    }

    Solace::Future<void> asyncWrite(Solace::ByteBuffer& src, UdpSocketReceiver& receiver) {
        return asyncWrite(src, receiver.getReceiver(), receiver.getServiceName(), src.remaining());
    }

    /**
     * Post an async write request to write specified amount of data into this IO object.
     *
     * @param src The provided source buffer to read data from.
     * @param bytesToWrite Amount of data (in bytes) to write from the buffer into this IO object.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     *
     * @note If the provided source buffer does not have requested amount of data - an exception is raised.
     */
    Solace::Future<void> asyncWrite(Solace::ByteBuffer& src,
                            const endpoint_type& receiver,
                            const Solace::String& serviceName,
                            std::size_t length);

private:
	asio::ip::udp::socket    _socket;
	asio::ip::udp::resolver  _resolver;

protected:
    friend class UdpsocketReceiver;
};


inline void swap(UdpSocket& lhs, UdpSocket& rhs) noexcept {
    lhs.swap(rhs);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_UDPSOCKET_HPP
