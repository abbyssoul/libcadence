/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
#pragma once
#ifndef CADENCE_ASIO_HELPER_LOCAL_HPP
#define CADENCE_ASIO_HELPER_LOCAL_HPP


#include "cadence/unixDomainEndpoint.hpp"
#include "cadence/networkEndpoint.hpp"

#include <asio/local/stream_protocol.hpp>


namespace cadence {

asio::local::stream_protocol::endpoint
toAsioLocalEndpoint(NetworkEndpoint const& addr, asio::error_code& ec);

NetworkEndpoint
fromAsioEndpoint(asio::local::stream_protocol::endpoint const& addr);

}  // end of namespace cadence
#endif  // CADENCE_ASIO_HELPER_LOCAL_HPP
