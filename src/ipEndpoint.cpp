/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/

#include "cadence/ipendpoint.hpp"


using namespace Solace;
using namespace cadence;


Solace::String IPEndpoint::toString() const {
    // FIXME(abbyssoul): Should use StringBuilder instead.
    return _ipAddress
            .concat(":")
            .concat(String::valueOf(_port));
}

