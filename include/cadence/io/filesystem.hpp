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
 * libcadence: virtual filesystem inteface
 *	@file		cadence/io/filesystem.hpp
 ******************************************************************************/
#pragma once
#ifndef CADENCE_IO_FILESYSTEM_CPP
#define CADENCE_IO_FILESYSTEM_CPP

#include "cadence/io/file.hpp"

#include <solace/path.hpp>

#include <memory>  // std::unique_ptr<>

namespace cadence {

/** Filesystem interface.
 * Filesystem is a hierarchical directory of objects.
 * One can think of it as a dictionary with string keys. Objects returned by filesyste are called 'file'
 * Note: virtual filesystem does not represents real platform filesystem but only defines a notion of
 * files and hierarchy on names. For plarform specific filesystem implementation @see PlatformFilesystem
 *
 * Note also that there is intentionally no close method on the Filesystem interface bacause it is resposiility
 * of the file object obatined via create/open to be closed after it is not longer needed. Please @see File for details
 *
 * TODO(abbyssoul): Most methods should return Result<>
 */
class Filesystem {
public:

    virtual ~Filesystem() = default;

    /**
     * Create a new file.
     *
     * @param path a path to the file to be created
     * @return A newly created file
     */
    virtual std::unique_ptr<File> create(Solace::Path const& path) = 0;

    /**
     * Open already existing file.
     *
     * @param path A path to the file to be opened
     * @return Opened file
     */
    virtual std::unique_ptr<File> open(Solace::Path const& path) = 0;

    /**
     * Unlink file by path.
     * Once no refernces to the file left the file is removed from filesystem.
     *
     * @param path A path to the file to be removed
     * @return True if file was removed
     */
    virtual bool unlink(Solace::Path const& path) = 0;

    /**
     * Check if a file identified by the path actually exists.
     *
     * @param path A path to the file to be checked
     * @return True if file exists
     */
    virtual bool exists(Solace::Path const& path) const = 0;

};

}  // End of namespace cadence
#endif  // CADENCE_IO_FILESYSTEM_CPP
