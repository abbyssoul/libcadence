/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/serialChannel.cpp
 *******************************************************************************/
#include <cadence/async/serial.hpp>
#include <solace/exception.hpp>

#include "asio_helper.hpp"



using namespace Solace;
using namespace Solace::IO;
using namespace cadence::async;


asio::serial_port_base::parity::type toAsioParity(Serial::Parity parity) {
    switch (parity) {
    case Serial::Parity::even:  return asio::serial_port_base::parity::even;
    case Serial::Parity::odd:   return asio::serial_port_base::parity::odd;
    case Serial::Parity::none:  return asio::serial_port_base::parity::none;
    default:
        raise<IllegalArgumentException>("Not supported parity type");
    }

    // Control actually never riches this point but some compiler are dumb enough to think it can.
    return asio::serial_port_base::parity::none;
}

asio::serial_port_base::stop_bits::type toAsioStopbits(Serial::Stopbits stopbits) {
    switch (stopbits) {
    case Serial::Stopbits::one:             return asio::serial_port_base::stop_bits::one;
    case Serial::Stopbits::two:             return asio::serial_port_base::stop_bits::two;
    case Serial::Stopbits::one_point_five:  return asio::serial_port_base::stop_bits::onepointfive;
    }

    // Control actually never riches this point but some compiler are dumb enough to think it can.
    return asio::serial_port_base::stop_bits::one;
}



asio::serial_port_base::flow_control::type toAsioFlowcontrol(Serial::Flowcontrol flowcontrol) {
    switch (flowcontrol) {
    case Serial::Flowcontrol::none:     return asio::serial_port_base::flow_control::none;
    case Serial::Flowcontrol::software: return asio::serial_port_base::flow_control::software;
    case Serial::Flowcontrol::hardware: return asio::serial_port_base::flow_control::hardware;
    }

    // Control actually never riches this point but some compiler are dumb enough to think it can.
    return asio::serial_port_base::flow_control::none;
}


class SerialChannel::SerialImpl {
public:
    SerialImpl(void* ioservice, const Path& file,
                                 uint32 baudrate,
                                 Serial::Bytesize bytesize,
                                 Serial::Parity parity,
                                 Serial::Stopbits stopbits,
                                 Serial::Flowcontrol flowcontrol) :
        _serial(*static_cast<asio::io_service*>(ioservice), file.toString().c_str())
    {
        _serial.set_option(asio::serial_port_base::baud_rate(baudrate));
        _serial.set_option(asio::serial_port_base::character_size(static_cast<uint32>(bytesize)));
        _serial.set_option(asio::serial_port_base::parity(toAsioParity(parity)));
        _serial.set_option(asio::serial_port_base::stop_bits(toAsioStopbits(stopbits)));
        _serial.set_option(asio::serial_port_base::flow_control(toAsioFlowcontrol(flowcontrol)));
    }



    Future<void>
    asyncRead(ByteBuffer& buffer, size_type bytesToRead) {
        Promise<void> promise;
        auto f = promise.getFuture();

        _serial.async_read_some(asio_buffer(buffer, bytesToRead),
            [pm = std::move(promise), &buffer](const asio::error_code& error, std::size_t length) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                buffer.advance(length);
                pm.setValue();
            }
        });

        return f;
    }


    Future<void>
    asyncWrite(ByteBuffer& buffer, size_type bytesToWrite) {
        Promise<void> promise;
        auto f = promise.getFuture();

        _serial.async_read_some(asio_buffer(buffer, bytesToWrite),
            [pm = std::move(promise), &buffer](const asio::error_code& error, std::size_t length) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                buffer.advance(length);
                pm.setValue();
            }
        });

        return f;
    }

    Result<void, Error> read(ByteBuffer& dest, size_type bytesToRead) {
        asio::error_code ec;

        const auto len = asio::read(_serial, asio_buffer(dest, bytesToRead), ec);
        if (ec) {
            return Err(fromAsioError(ec));
        } else {
            dest.advance(len);
        }

        return Ok();
    }

    Result<void, Error> write(ByteBuffer& src, size_type bytesToWrite) {
        asio::error_code ec;

        const auto len = asio::write(_serial, asio_buffer(src, bytesToWrite), ec);
        if (ec) {
            return Err(fromAsioError(ec));
        } else {
            src.advance(len);
        }

        return Ok();
    }


private:
    asio::serial_port _serial;
};


SerialChannel::~SerialChannel()
{
}


SerialChannel::SerialChannel(EventLoop& ioContext, const Path& file,
                             uint32 baudrate,
                             Serial::Bytesize bytesize,
                             Serial::Parity parity,
                             Serial::Stopbits stopbits,
                             Serial::Flowcontrol flowcontrol) :
    Channel(ioContext),
    _pimpl(std::make_unique<SerialImpl>(ioContext.getIOService(), file,
                                        baudrate,
                                        bytesize,
                                        parity,
                                        stopbits,
                                        flowcontrol))
{
}


SerialChannel::SerialChannel(SerialChannel&& rhs) :
    Channel(std::move(rhs)),
    _pimpl(std::move(rhs._pimpl))
{
}


SerialChannel&
SerialChannel::swap(SerialChannel& rhs) noexcept {
    using std::swap;
    swap(_pimpl, rhs._pimpl);

    return *this;
}


Future<void>
SerialChannel::asyncRead(ByteBuffer& buffer, size_type bytesToRead) {
    return _pimpl->asyncRead(buffer, bytesToRead);
}


Future<void> SerialChannel::asyncWrite(ByteBuffer& buffer, size_type bytesToWrite) {
    return _pimpl->asyncWrite(buffer, bytesToWrite);
}


Result<void, Error> SerialChannel::read(ByteBuffer& dest, size_type bytesToRead) {
    return _pimpl->read(dest, bytesToRead);
}

Result<void, Error> SerialChannel::write(ByteBuffer& src, size_type bytesToWrite) {
    return _pimpl->write(src, bytesToWrite);
}
