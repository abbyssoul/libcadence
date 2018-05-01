/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/

#include <cadence/asyncServer.hpp>
#include <cadence/ipendpoint.hpp>
#include <cadence/unixDomainEndpoint.hpp>
#include <cadence/async/tcpsocket.hpp>
#include <cadence/version.hpp>

#include <solace/memoryManager.hpp>
#include <solace/uuid.hpp>
#include <solace/cli/parser.hpp>


#include <iostream>


using namespace Solace;
using namespace cadence;
using namespace cadence::async;



int main(int argc, const char **argv) {
    uint16 serverPort = 5640;
    StringView serverEndpoint("127.0.0.1");

    auto res = cli::Parser("libcadence/async_server", {
                            cli::Parser::printHelp(),
                            cli::Parser::printVersion("async_clent", cadence::getBuildVersion()),

                            {{"p", "port"}, "Server port", &serverPort},
                            {{"h", "host"}, "Server listen address", &serverEndpoint}
                           })
            .parse(argc, argv);

    if (!res) {
        const auto& e = res.getError();

        if (e) {
            std::cerr << "Error: " <<  e << std::endl;

            return EXIT_FAILURE;
        } else {
            std::cerr << e << std::endl;

            return EXIT_SUCCESS;
        }
    }

    MemoryManager memManager(3 * 8*1024);


    char simpleMessage[] = "Hello data!\n";
    ByteBuffer dataBuffer(memManager.create(128));

    auto regionId = UUID::random();
    dataBuffer << regionId;
    dataBuffer << Solace::MemoryView::size_type(32);

    auto region2 = UUID::random();

    EventLoop iocontext;
    AsyncServer server(iocontext, memManager);
    server.mount(Path("simple"), std::make_shared<DataNode>(wrapMemory(simpleMessage)));
    server.mount(Path("regions"), std::make_shared<DirectoryNode>());
    server.mount(Path({"regions", "test1"}), std::make_shared<DataNode>(dataBuffer.viewWritten()));
    server.mount(Path({"regions", region2.toString()}), std::make_shared<DataNode>(region2.view()));

    /*
    if (serverEndpoint.startsWith('/')) {
        UnixEndpoint endpoint(serverEndpoint);
        server.startListen(endpoint).
                orElse([](Error&& e) {
                    std::cerr << e << std::endl;
                });
    } else  */
    {
        IPEndpoint ipEndpoint(serverEndpoint, serverPort);
        server.startListen(ipEndpoint).
                orElse([](Error&& e) {
                    std::cerr << e << std::endl;
                });
    }

    // Run event loop
    iocontext.run();

    return EXIT_SUCCESS;
}
