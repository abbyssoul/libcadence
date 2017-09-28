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


using namespace Solace;
using namespace cadence::async;


bool EventLoop::poll() {
	return _io_service.poll();
}

void EventLoop::run() {
	_io_service.run();
}

void EventLoop::runFor(int msec) {
	_io_service.run_for(std::chrono::milliseconds(msec));
}

bool EventLoop::isStopped() {
	return _io_service.stopped();
}

void EventLoop::reset() {
	_io_service.reset();
}

void EventLoop::stop() {
	_io_service.stop();
}
