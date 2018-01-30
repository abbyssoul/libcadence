/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Async Resource server client
 *	@file		cadence/asyncClient.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNCAPP_HPP
#define CADENCE_ASYNCAPP_HPP

#include <solace/memoryManager.hpp>
#include <solace/framework/application.hpp>     // Solace Application Framework

#include <cadence/async/eventloop.hpp>
#include <cadence/async/signalSet.hpp>


namespace cadence {


/**
 * Helper class to be used as a base class for async applications.
 * It generalise application configuraion and setup steps.
 *
 */
class AsyncApp :
        public Solace::Framework::Application {

public:

    virtual ~AsyncApp() = default;

    AsyncApp(const char* name,
              const Solace::Version& version,
              Solace::MemoryManager::size_type memSizeHint);

    using Application::init;

    /**
     * Start application async IO loop.
     * All initialization steps must be done in @see init() method.
     *
     * Usually application create a set of async tasks during init and hand it over to `run` method to execute in a loop.
     * New async tasks can be spawed as in the loop and will be added to the queue.
     *
     * @return Result of the run to be used as process exit code or an error.
     */
    Solace::Result<int, Solace::Error> run();

    /**
     * Stop async loop.
     * Note: This will not interrupt currently running task thus exicution continues till but no more tasks will be
     * picked from the async queue.
     *
     * This method always succeed (as it only changes the boolean flag).
     * It is safe to call this method muliple times.
     */
    void stop();

    /**
     * Get application name string.
     * @return Application name.
     */
    const char* getName() const noexcept { return _name; }

protected:

    Solace::MemoryManager&          getMemoryManager()  noexcept    { return _memoryManager; }
    async::EventLoop&               getEventLoop()      noexcept    { return _eventLoop; }
    async::SignalSet&               getSignalSet()      noexcept    { return _signalSet; }

private:

    const char*                         _name;
//    Solace::Config                      _config;

    Solace::MemoryManager               _memoryManager;

    async::EventLoop            _eventLoop;
    async::SignalSet            _signalSet;

};

}  // End of namespace cadence
#endif  // CADENCE_ASYNCAPP_HPP
