/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/pipe.cpp
 *******************************************************************************/
#include <cadence/async/pipe.hpp>

#include <solace/exception.hpp>

#include <unistd.h>
#include <fcntl.h>

#include "asio.hpp"


using namespace Solace;
using namespace cadence::async;


void createNonblockingPipe(int* fds) {
	const auto r = pipe2(fds, O_NONBLOCK);
	if (r < 0) {
		Solace::raise < IOException > (errno, "pipe2");
	}
}


Pipe::Pipe(EventLoop& ioContext) :
    _in(ioContext.getIOService()),
    _out(ioContext.getIOService())
{
	int fds[2] = { 0 };
	createNonblockingPipe(fds);

	_in.assign(fds[0]);
	_out.assign(fds[1]);
}

Pipe::Pipe(Pipe&& rhs) :
    _in(std::move(rhs._in)),
    _out(std::move(rhs._out))
{
}


Future<void> Pipe::asyncRead(ByteBuffer& dest, std::size_t bytesToRead) {
    Promise<void> promise;
    auto f = promise.getFuture();

    _in.async_read_some(asio_buffer(dest, bytesToRead),
        [&dest, pm = std::move(promise)](const asio::error_code error, std::size_t length) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
            dest.advance(length);
            pm.setValue();
        }
	});

	return f;
}


Future<void> Pipe::asyncWrite(ByteBuffer& src, std::size_t bytesToWrite) {
    Promise<void> promise;
    auto f = promise.getFuture();

    _out.async_write_some(asio_buffer(src, bytesToWrite),
        [&src, pm = std::move(promise)](const asio::error_code error, std::size_t length) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
            src.advance(length);
            pm.setValue();
        }
	});

	return f;
}
