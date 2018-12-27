/*
*  Copyright 2016 Ivan Ryabov
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
 * libcadence: Interface of selectable / pollable objects
 *	@file		cadence/io/selectable.hpp
 ******************************************************************************/
#pragma once
#ifndef CADENCE_IO_SELECTABLE_HPP
#define CADENCE_IO_SELECTABLE_HPP

namespace cadence {

/**
 * A base interface for an object that can be polled by select/poll/epoll
 * family of functions
 */
class ISelectable {
public:
    //!< File Id is defined as int by POSIX
    using poll_id = int;

    static const poll_id InvalidFd;

public:

    /**
     * Default virtual destructor is fine for a base virtual class.
     */
	virtual ~ISelectable() = default;

    /**
     * Get a 'native' file id of this object to be used by poll/select functions.
     * @return
     */
    virtual poll_id getSelectId() const = 0;
};


}  // namespace cadence
#endif  // CADENCE_IO_SELECTABLE_HPP
