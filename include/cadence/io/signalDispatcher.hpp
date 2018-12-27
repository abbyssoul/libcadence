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
 * libcadence: Signal Dispatcher for POSIX platform
 *	@file		cadence/io/signalDispatcher.hpp
 *	@brief		Posix signal dispatcher class
 ******************************************************************************/
#pragma once
#ifndef CADENCE_SIGNALDISPATCHER_HPP
#define CADENCE_SIGNALDISPATCHER_HPP

#include <solace/types.hpp>

#include <functional>  // std::function<>


namespace cadence {

/**
 * Conveniece class to manage POSIX signal subbscription in OOP manner.
 */
class SignalDispatcher {
public:

    /**
     * One process can only have one signal dispatcher.
     */
    static SignalDispatcher& getInstance();

    /**
      * Attach given handler to the give posix signal.
      *
      * @param signalNumber POSIX signal number
      * @param signalHandler Function to be called when the signal received
      *
      * TODO(abbyssoul): Return some kind of a descriptor to be used for 'removeHandler'.
     */
    void attachHandler(int signalNumber, const std::function<void(int)>& signalHandler);


protected:

    /**
      * Default constructor
     */
    SignalDispatcher();

    /**
     * Nothing here, really
     */
    ~SignalDispatcher();

    /**
      * Copy is not allowed. It makes no sense.
     */
    SignalDispatcher(const SignalDispatcher&) = delete;

    //!< Delete copy assignment as well - makes no sense.
    const SignalDispatcher& operator= (const SignalDispatcher&) = delete;

};

}  // namespace cadence
#endif  // CADENCE_SIGNALDISPATCHER_HPP
