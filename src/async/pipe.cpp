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
#include "cadence/async/pipe.hpp"

#include "asio_helper.hpp"

#include <asio/posix/stream_descriptor.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

#include <solace/exception.hpp>


using namespace Solace;
using namespace cadence;
using namespace cadence::async;




void createNonblockingPipe(int* fds) {
    const auto r = pipe2(fds, O_NONBLOCK);
    if (r < 0) {
        Solace::raise < IOException > (errno, "pipe2");
    }
}


class Pipe::PipeImpl {
public:

    PipeImpl(void* ioservice) :
        _in(asAsioService(ioservice)),
        _out(asAsioService(ioservice))
    {
        int fds[2] = { 0 };
        createNonblockingPipe(fds);

        _in.assign(fds[0]);
        _out.assign(fds[1]);
    }


    Future<void> asyncRead(ByteWriter& dest, std::size_t bytesToRead) {
        Promise<void> promise;
        auto f = promise.getFuture();

        _in.async_read_some(asio_buffer(dest, bytesToRead),
            [&dest, pm = std::move(promise)](const asio::error_code error, std::size_t length) mutable {
            if (error) {
                pm.setError(fromAsioError(error, "asyncRead"));
            } else {
                dest.advance(length);
                pm.setValue();
            }
        });

        return f;
    }


    Future<void> asyncWrite(ByteReader& src, std::size_t bytesToWrite) {
        Promise<void> promise;
        auto f = promise.getFuture();

        _out.async_write_some(asio_buffer(src, bytesToWrite),
            [&src, pm = std::move(promise)](const asio::error_code error, std::size_t length) mutable {
            if (error) {
                pm.setError(fromAsioError(error, "asyncWrite"));
            } else {
                src.advance(length);
                pm.setValue();
            }
        });

        return f;
    }


    Result<void, Error> read(ByteWriter& dest, size_type bytesToRead) {
        asio::error_code ec;

        const auto len = asio::read(_in, asio_buffer(dest, bytesToRead), ec);
        if (ec) {
            return Err(fromAsioError(ec, "read"));
        } else {
            dest.advance(len);
        }

        return Ok();
    }

    Result<void, Error> write(ByteReader& src, size_type bytesToWrite) {
        asio::error_code ec;

        const auto len = asio::write(_out, asio_buffer(src, bytesToWrite), ec);
        if (ec) {
            return Err(fromAsioError(ec, "write"));
        } else {
            src.advance(len);
        }

        return Ok();
    }


    void cancel() {
        _in.cancel();
        _out.cancel();
    }

    void close() {
        _in.close();
        _out.close();
    }

    bool isOpen() const {
        return _in.is_open() && _out.is_open();
    }

    bool isClosed() const {
        return !isOpen();
    }

private:
    asio::posix::stream_descriptor _in;
    asio::posix::stream_descriptor _out;
};


Pipe::~Pipe() = default;

Pipe::Pipe(EventLoop& ioContext) :
    Channel(ioContext),
    _pimpl(std::make_unique<PipeImpl>(ioContext.getIOService()))
{
}


Pipe::Pipe(Pipe&& rhs) noexcept
    : Channel(std::move(rhs))
    , _pimpl(std::move(rhs._pimpl))
{}


Pipe& Pipe::swap(Pipe& rhs) noexcept {
    using std::swap;
    swap(_pimpl, rhs._pimpl);

    return *this;
}


Future<void> Pipe::asyncRead(ByteWriter& dest, size_type bytesToRead) {
    return _pimpl->asyncRead(dest, bytesToRead);
}


Future<void> Pipe::asyncWrite(ByteReader& src, size_type bytesToWrite) {
    return _pimpl->asyncWrite(src, bytesToWrite);
}


Result<void, Error> Pipe::read(ByteWriter& dest, size_type bytesToRead) {
    return _pimpl->read(dest, bytesToRead);
}


Result<void, Error> Pipe::write(ByteReader& src, size_type bytesToWrite) {
    return _pimpl->write(src, bytesToWrite);
}


void Pipe::cancel() {
    _pimpl->cancel();
}

void Pipe::close() {
    _pimpl->close();
}

bool Pipe::isOpen() const {
    return _pimpl->isOpen();
}

bool Pipe::isClosed() const {
    return _pimpl->isClosed();
}
