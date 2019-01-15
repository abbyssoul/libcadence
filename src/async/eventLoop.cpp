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

#include <asio/io_context.hpp>


using namespace cadence::async;


class EventLoop::EventloopImpl {
public:

    EventLoop::size_type poll() {
        return _io_service.poll();
    }

    void run() {
        _io_service.run();
    }

    void runFor(int msec) {
        _io_service.run_for(std::chrono::milliseconds(msec));
    }

    bool isStopped() const {
        return _io_service.stopped();
    }

    void reset() {
        _io_service.reset();
    }

    void stop() {
        _io_service.stop();
    }

    void onForkChild() {
        _io_service.notify_fork(asio::io_context::fork_event::fork_child);
    }

    void onForkParent() {
        _io_service.notify_fork(asio::io_context::fork_event::fork_parent);
    }

    asio::io_context* getIOService() {
        return &_io_service;
    }

private:

    asio::io_context _io_service;
};



EventLoop::~EventLoop() = default;


EventLoop::EventLoop()
    : _pimpl(std::make_unique<EventloopImpl>())
{
}

EventLoop::size_type EventLoop::poll() {
    return _pimpl->poll();
}

void EventLoop::run() {
    _pimpl->run();
}

void EventLoop::runFor(int msec) {
    _pimpl->runFor(msec);
}

bool EventLoop::isStopped() const noexcept {
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
