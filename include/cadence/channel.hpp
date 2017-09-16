/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
#pragma once
#ifndef ALISS_CADENCE_CHANNEL_HPP
#define ALISS_CADENCE_CHANNEL_HPP

#include <solace/io/async/channel.hpp>


namespace Aliss { namespace cadence {

class Channel: public Solace::IO::async::Channel {

};

}  // end of namespace Cadence
}  // end of namespace Aliss
#endif  // ALISS_CADENCE_CHANNEL_HPP
