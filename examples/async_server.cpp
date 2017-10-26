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
#include <cadence/async/tcpsocket.hpp>
#include <cadence/version.hpp>

#include <solace/memoryManager.hpp>
#include <solace/framework/commandlineParser.hpp>


#include <iostream>

using namespace Solace;
using namespace Solace::Framework;
using namespace cadence;
using namespace cadence::async;



int main(int argc, const char **argv) {
    uint16 serverPort = 5640;
    String serverEndpoint("127.0.0.1");

    auto res = CommandlineParser("libcadence/async_server", {
                            CommandlineParser::printHelp(),
                            CommandlineParser::printVersion("async_clent", cadence::getBuildVersion()),
                            {'p', "port", "Server port", &serverPort},
                            {'h', "host", "Resource server endpoint", &serverEndpoint}})
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

    IPEndpoint ipEndpoint(serverEndpoint, serverPort);
    MemoryManager memManager(3 * 8*1024);
    EventLoop iocontext;

    AsyncServer server(iocontext, memManager);
    server.startListen(ipEndpoint).
            orElse([](Error&& e) {
                std::cerr << e << std::endl;
            });

    // Run event loop
    iocontext.run();

    return EXIT_SUCCESS;
}
