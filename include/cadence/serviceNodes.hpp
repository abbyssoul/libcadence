/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Async Resource Server
 *	@file		cadence/asyncServer.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_SERVICENODES_HPP
#define CADENCE_SERVICENODES_HPP

#include <solace/result.hpp>
#include <solace/error.hpp>
#include <solace/memoryManager.hpp>
#include <solace/byteBuffer.hpp>

#include <memory>
#include <map>


namespace cadence {


/**
 * Base class for Nodes served by the server.
 * Node is an element of the hierarchy. It can be a resource such as file or a directory.
 * Files can be a data blobs such as files of a disk or synthetic resources (like proc fs)
 */
class Node {
public:
    virtual ~Node() = default;

    /**
     * Mount a new node under a given path.
     * @param pathSegment A path segment to mount a node under. Note it is a name not a hierarchical path.
     * @param node Node to mount under a given path.
     * @return Success or error if failed to mount.
     */
    virtual Solace::Result<void, Solace::Error>
    mount(const Solace::String& pathSegment, std::shared_ptr<Node>&& node);

    /**
     * Attempt to resolve a non-hierarchical name into a node.
     * @param pathSegment A name to resolve.
     * @return A shared pointer to a node if the name resolves or an error othewise.
     */
    virtual Solace::Result<std::shared_ptr<Node>, Solace::Error>
    walk(const Solace::String& pathSegment);

    /**
     * Test if the node is walkable, that is node can have children.
     * @return True if node can have children.
     */
    virtual bool isWalkable() const noexcept {
        return false;
    }

    /**
     * Get content version of the node.
     * By convention, version number must be updated each time node's content changes.
     * For 'file'-like nodes that means when node is written to.
     * @return Version number of node's content.
     */
    virtual Solace::uint32 getVersion() const noexcept {
        return 0;
    }

    /**
     * Open node for operations specified by mode bit-field.
     * @param uname 'User name' - an indentier of the operation principle.
     * @param mode A bit field of the operations to open the node for.
     * @return A result of the open operation - success or an error.
     */
    virtual Solace::Result<void, Solace::Error>
    open(const Solace::String& uname, Solace::byte mode) = 0;

    /**
     * Close the node.
     * @param uname 'User name' - an indentier of the operation principle.
     * @return A result of the operation - success or an error.
     */
    virtual Solace::Result<void, Solace::Error>
    close(const Solace::String& uname);

    virtual Solace::Result<void, Solace::Error>
    read(Solace::uint32 count, Solace::uint64 offset, Solace::ByteBuffer& buffer) = 0;

    virtual Solace::Result<void, Solace::Error>
    write(Solace::ImmutableMemoryView data, Solace::uint64 offset) = 0;
};


/**
 * A base class for directory node.
 * Directory is a collection of other nodes. By convention the directory has no content
 * (other then references to child nodes).
 */
class DirectoryNode : public Node {
public:

    Solace::Result<void, Solace::Error>
    mount(const Solace::String& pathSegment, std::shared_ptr<Node>&& node) override;

    Solace::Result<std::shared_ptr<Node>, Solace::Error>
    walk(const Solace::String& pathSegment) override;

    bool isWalkable() const noexcept override {
        return true;
    }

    Solace::Result<void, Solace::Error>
    open(const Solace::String& uname, Solace::byte mode) override;

    Solace::Result<void, Solace::Error>
    read(Solace::uint32 count, Solace::uint64 offset, Solace::ByteBuffer& buffer) override;

    Solace::Result<void, Solace::Error>
    write(Solace::ImmutableMemoryView data, Solace::uint64 offset) override;

private:

    std::map<Solace::String, std::shared_ptr<Node>> _mounts;
};



/**
 * Simple chunk of data.
 */
class DataNode : public Node {
public:
    DataNode(Solace::MemoryView&& data);

    Solace::Result<void, Solace::Error>
    open(const Solace::String& uname, Solace::byte mode) override;

    Solace::Result<void, Solace::Error>
    read(Solace::uint32 count, Solace::uint64 offset, Solace::ByteBuffer& buffer) override;

    Solace::Result<void, Solace::Error>
    write(Solace::ImmutableMemoryView data, Solace::uint64 offset) override;

private:

    Solace::MemoryView _data;
};


}  // End of namespace cadence
#endif // CADENCE_SERVICENODES_HPP
