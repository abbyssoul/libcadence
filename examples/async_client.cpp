/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/

#include <cadence/asyncClient.hpp>
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


Future<void> runTestSession(const String& rootName, const String& userName, const String& dir, AsyncClient& client) {

    return client.beginSession(rootName, userName)
            .then([&client]() {
                std::cout << "Session established" << std::endl;
            })
            .then([&client, &dir]() {
                return client.list(Path::parse(dir))
                        .then([](Solace::Array<Solace::Path>&& list) {
                            std::cout << "ls> " << std::endl;
                            for (auto& path : list) {
                                std::cout << path.toString() << std::endl;
                            }
                            std::cout << std::endl;
                        });
            })
            .then([&client]() {
                return client.read(Path({"data", "updated"}))
                        .then([](Solace::MemoryView&& data) {
                            std::cout << "read> \'";
                            std::cout.write(data.dataAs<const char>(), data.size());
                            std::cout << "\'" << std::endl;
                        });
            })
            .onError([](Solace::Error&& err) {
                std::cerr << "Error: " << err.toString() << std::endl;
            });
}


int main(int argc, const char **argv) {

    P9Protocol::size_type bufferSize = P9Protocol::MAX_MESSAGE_SIZE;
    uint16 serverPort = 5640;
    String userName;
    String rootName;
    String serverEndpoint("127.0.0.1");
    String dir("data");

    auto res = CommandlineParser("libcadence/async_client", {
                            CommandlineParser::printHelp(),
                            CommandlineParser::printVersion("async_clent", cadence::getBuildVersion()),
                            {'p', "port", "Server port", &serverPort},
                            {'u', "user", "User name to Authenticate as", &userName},
                            {'r', "root", "Resource root to attach to", &rootName},
                            {'m', "msgSize", "Maximum message size", &bufferSize},
                            {'h', "host", "Resource server endpoint", &serverEndpoint}},
                        {{"dir", "Directory to explore", &dir}})
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
    MemoryManager memManager(3 * bufferSize);
    EventLoop iocontext;

    auto socket = std::make_unique<TcpSocket>(iocontext);

    auto result = socket->connect(ipEndpoint);

    AsyncClient client(std::move(socket), memManager);
    result.then([&]() {
                runTestSession(rootName, userName, dir, client);
            }).orElse([&ipEndpoint](Error&& er) {
                std::cout << "Failed to connect to '" << ipEndpoint << "': " << er.toString() << std::endl;
            });


    // Run event loop
    iocontext.run();

    return EXIT_SUCCESS;
}
