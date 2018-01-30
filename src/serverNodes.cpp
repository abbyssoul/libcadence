/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/

#include "cadence/serviceNodes.hpp"
#include "cadence/protocols/9p2000x.hpp"



using namespace Solace;
using namespace cadence;



// Defined in asyncServerSession:
P9Protocol::Stat nodeStats(const String& name, const std::shared_ptr<Node>& node);




Node::~Node() {
}


Result<void, Error>
Node::mount(const String& , std::shared_ptr<Node>&& ) {
    return Err(Error("Not a directory"));
}


Result<std::shared_ptr<Node>, Error>
Node::walk(const String& ) {
    return Err(Error("Not a directory"));
}


Result<void, Error>
Node::close(const String& /*uname*/) {
    return Ok();
}




Result<void, Error>
DirectoryNode::mount(const Solace::String& pathSegment, std::shared_ptr<Node>&& node) {
    auto it = _mounts.find(pathSegment);
    if (it == _mounts.end()) {
        _mounts.emplace(pathSegment, std::move(node));

        return Ok();
    } else {
        return Err(Error("Can't mount: path already exist"));
    }
}


Result<std::shared_ptr<Node>, Error>
DirectoryNode::walk(const Solace::String& pathSegment) {
    auto it = _mounts.find(pathSegment);
    if (it == _mounts.end())
        return Err(Error("not found"));

    return Ok(it->second);
}

Result<void, Error>
DirectoryNode::open(const String& uname, byte mode) {
    // TODO(abbyssoul): Check ACL if user identified by uname can has access.

    return Ok();
}



Result<void, Error>
DirectoryNode::read(uint32 count, uint64 offset, ByteBuffer& buffer) {
    P9Protocol::Encoder encoder(buffer);

    uint64 bytesTraversed = 0;
    uint32 bytesEncoded = 0;

    for (auto entry : _mounts) {
        P9Protocol::Stat stat(nodeStats(entry.first, entry.second));

        const auto protoSize = encoder.protocolSize(stat);
        // Keep count of how many data we have traversed.
        bytesTraversed += protoSize;
        if (bytesTraversed <= offset)  // Client is only interested in data pass the offset.
            continue;

        // Keep track of much data will end up in a buffer to prevent overflow.
        bytesEncoded += protoSize;
        if (bytesEncoded > count)
            break;

        // Only encode the data if we have some room left, as specified by 'count' arg.
        encoder.encode(stat);
    }

    return Ok();
}


Result<void, Error>
DirectoryNode::write(ImmutableMemoryView /*data*/, uint64 /*offset*/) {
    return Err(Error("Write not allowed"));
}


DataNode::DataNode(Solace::MemoryView&& data) :
    _data(std::move(data))
{
}

Result<void, Error>
DataNode::open(const String& , byte ) {
    return Ok();
}


Result<void, Error>
DataNode::read(uint32 count, uint64 offset, ByteBuffer& buffer) {
    if (offset >= _data.size()) // It's ok to try to read beyond the file end. We return 0 to signal EOF.
        return Ok();

    const auto slise = _data.slice(offset, _data.size());
    buffer.write(slise);

    return Ok();
}


Result<void, Error>
DataNode::write(ImmutableMemoryView data, uint64 offset) {
    _data.write(data, offset);

    return Ok();
}

