/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/asyncSystemErrorDomain.cpp
*******************************************************************************/
#include "asynErrorDomain.hpp"

#include <solace/errorDomain.hpp>

#include <asio/error.hpp>

using namespace Solace;
using namespace cadence;
using namespace cadence::async;


const AtomValue cadence::async::kAsyncErrorCatergory = atom("pasy");


class AsyncErrorDomain :
    public ErrorDomain {
public:

    StringView getName() const noexcept override {
        return StringLiteral("AsyncErrorDomain");
    }


    StringView getMessage(int code) const noexcept override {
//        asio::error_code

        return "";
    }

};

static const AsyncErrorDomain kAsyncErrorDomain;
static auto const asyncErrorRego = registerErrorDomain(kAsyncErrorCatergory, kAsyncErrorDomain);
