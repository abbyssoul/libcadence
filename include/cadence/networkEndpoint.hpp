/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Abstract network endpoint type
 * @file        cadence/networkEndpoint.hpp
 ******************************************************************************/
#pragma once
#ifndef CADENCE_NETWORKENDPOINT_HPP
#define CADENCE_NETWORKENDPOINT_HPP

#include "ipendpoint.hpp"
#include "unixDomainEndpoint.hpp"

#include <variant>


namespace cadence {

using NetworkEndpoint = std::variant<IPEndpoint, UnixEndpoint>;

/**
 * An abstract class to represent network address.
 *//*
class NetworkEndpoint {
public:


    //!< @see Solace::IFormattable::toString
     Solace::String toString() const;
};*/

}  // End of namespace cadence
#endif  // CADENCE_NETWORKENDPOINT_HPP
