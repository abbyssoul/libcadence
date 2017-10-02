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
#ifndef CADENCE_NETADDRESS_HPP
#define CADENCE_NETADDRESS_HPP

#include <solace/string.hpp>


namespace cadence {

/**
 * TODO(abbyssoul): document this class
 */
class IPEndpoint {
public:

    IPEndpoint(const Solace::String& address, Solace::uint16 port) :
        _ipAddress(address),
        _port(port)
    {
    }

    const Solace::String& getAddress() const {
        return _ipAddress;
    }

    Solace::uint16 getPort() const {
        return _port;
    }

private:
    // FIXME: Should use IPAddress type instead of the String
    Solace::String  _ipAddress;
    Solace::uint16  _port;
};

}  // End of namespace cadence
#endif  // CADENCE_NETADDRESS_HPP
