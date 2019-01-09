/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
#pragma once
#ifndef CADENCE_ASIO_HELPER_HPP
#define CADENCE_ASIO_HELPER_HPP

#include "asynErrorDomain.hpp"

#include <solace/error.hpp>
#include <solace/byteReader.hpp>
#include <solace/byteWriter.hpp>

#include <asio/io_context.hpp>
#include <asio/buffer.hpp>


namespace cadence {

inline
asio::io_context& asAsioService(void* ioservice) {
    return *static_cast<asio::io_context*>(ioservice);
}

inline
Solace::Error fromAsioError(asio::error_code const& err, Solace::StringLiteral tag) {
    return makeError(AsyncError::AsyncError, err.value(), tag);
}


inline
asio::mutable_buffer asio_buffer(Solace::ByteWriter& dest, Solace::ByteWriter::size_type bytes) {
    return asio::buffer(dest.viewRemaining().slice(0, bytes).dataAddress(), bytes);
}

inline
asio::const_buffer asio_buffer(Solace::ByteReader& dest, Solace::ByteReader::size_type bytes) {
    return asio::buffer(dest.viewRemaining().slice(0, bytes).dataAddress(), bytes);
}

}  // end of namespace cadence
#endif  // CADENCE_ASIO_HELPER_HPP
