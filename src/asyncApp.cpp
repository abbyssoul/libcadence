/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/

#include "cadence/asyncApp.hpp"
#include <solace/exception.hpp>


#include <signal.h>     // Definition of well know process signals

// Error logging output
#include <iostream>


using namespace Solace;
using namespace cadence;


AsyncApp::AsyncApp(const char* name, const Version& version, MemoryManager::size_type memSizeHint) :
    Framework::Application(version),

    _name(name),
    _memoryManager(memSizeHint),
    _eventLoop(),
    _signalSet(_eventLoop, {SIGINT, SIGTERM})
{
}


Result<int, Solace::Error>
AsyncApp::run() {
    // Following P10 we don't want any memory allocations after init.
    _memoryManager.lock();

    try {
        _eventLoop.run();
    } catch (const Exception& ex) {

        return Err(Solace::Error(ex.toString().c_str(), EXIT_FAILURE));
    }

    return Ok(EXIT_SUCCESS);
}


void AsyncApp::stop() {
    std::cout << "Stopping application" << std::endl;

    // Stop async event loop. No more outstanding tasks will be picked from the queue.
    _eventLoop.stop();

    // Note: We don't unlock memory manager as only allocation is locked, and it doesn't make sense to allocate
    // on de-init.

    std::cout << "Application stopped" << std::endl;
}

