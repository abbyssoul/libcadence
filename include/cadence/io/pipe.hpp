/*
*  Copyright 2016 Ivan Ryabov
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*/
/*******************************************************************************
 * libcadence: Unix unnamed pipe wrapper
 *	@file		cadence/io/pipe.hpp
 ******************************************************************************/
#pragma once
#ifndef CADENCE_IO_PIPE_HPP
#define CADENCE_IO_PIPE_HPP

#include "cadence/io/duplex.hpp"


namespace cadence {

/**
 * A wrapper for Unix unnamed pipe
 */
class Pipe :
        public Duplex {
public:
    using Duplex::poll_id;

public:

    Pipe();

    /**
     * Construct a new pipe object from an already opened fds
     * @param inFid FD to be a read end of the pipe
     * @param outFid FD to be a write end of the pipe
     */
    Pipe(poll_id inFid, poll_id outFid): Duplex(inFid, outFid)
    {}

    Pipe(Duplex&& duplex);
    Pipe(Pipe&& duplex) noexcept;

};

}  // End of namespace cadence
#endif  // CADENCE_IO_PIPE_HPP
