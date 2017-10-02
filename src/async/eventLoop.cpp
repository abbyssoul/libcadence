/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/eventLoop.cpp
 *******************************************************************************/
#include "cadence/async/eventloop.hpp"

#include <chrono>

#include "asio.hpp"


using namespace Solace;
using namespace cadence::async;


class EventLoop::EventloopImpl {
public:
    EventloopImpl():
        _io_service()
    {}

    bool poll() {
        return _io_service.poll();
    }

    void run() {
        _io_service.run();
    }

    void runFor(int msec) {
        _io_service.run_for(std::chrono::milliseconds(msec));
    }

    bool isStopped() {
        return _io_service.stopped();
    }

    void reset() {
        _io_service.reset();
    }

    void stop() {
        _io_service.stop();
    }

    asio::io_service* getIOService() {
        return &_io_service;
    }

private:

    asio::io_service _io_service;
};


EventLoop::~EventLoop()
{
}


EventLoop::EventLoop() :
    _pimpl(std::make_unique<EventloopImpl>())
{

}

EventLoop& EventLoop::swap(EventLoop& rhs) noexcept {
    using std::swap;
    swap(_pimpl, rhs._pimpl);

    return rhs;
}


bool EventLoop::poll() {
    return _pimpl->poll();
}

void EventLoop::run() {
    _pimpl->run();
}

void EventLoop::runFor(int msec) {
    _pimpl->runFor(msec);
}

bool EventLoop::isStopped() {
    return _pimpl->isStopped();
}

void EventLoop::reset() {
    _pimpl->reset();
}

void EventLoop::stop() {
    _pimpl->stop();
}

void* EventLoop::getIOService() noexcept {
    return _pimpl->getIOService();
}
