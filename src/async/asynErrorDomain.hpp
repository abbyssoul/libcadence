/*
*  Copyright 2016-2018 Ivan Ryabov
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*/
/*******************************************************************************
 * libcadence: Async error
 *	@file		async/asyncErrorDomain.hpp
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNCERRORDOMAIN_HPP
#define CADENCE_ASYNCERRORDOMAIN_HPP

#include <solace/atom.hpp>
#include <solace/error.hpp>
#include <solace/errorDomain.hpp>


namespace cadence::async {


/** Category of async errors.
 *
 */
extern const Solace::AtomValue kAsyncErrorCatergory;

enum class AsyncError: int {
    AsyncSystemError
};

[[nodiscard]] inline
Solace::Error makeError(AsyncError SOLACE_UNUSED(errType), int errCode, Solace::StringLiteral tag) noexcept {
    return {kAsyncErrorCatergory, errCode, tag};
}

}  // End of namespace cadence::async
#endif  // CADENCE_ASYNCERRORDOMAIN_HPP
