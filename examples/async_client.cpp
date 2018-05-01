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
#include <solace/cli/parser.hpp>


#include <iostream>

using namespace Solace;
using namespace cadence;
using namespace cadence::async;


Future<void>
runTestSession(const String& rootName, const String& userName, const String& dir, AsyncClient& client) {

    return client.beginSessionAsync(rootName, userName)
            .then([]() {
                std::cout << "Session established" << std::endl;
            })
            .then([&client, &dir]() {
                return client.listAsync(Path::parse(dir))
                        .then([](Solace::Array<Solace::Path>&& list) {
                            std::cout << "ls> " << std::endl;
                            for (auto& path : list) {
                                std::cout << path.toString() << std::endl;
                            }
                            std::cout << std::endl;
                        });
            })
            .then([&client]() {
                return client.readAsync(Path({"data", "updated"}))
                        .then([](AsyncClient::TransactionalMemoryView&& txdata) {
                            std::cout << "read> \'";
                            std::cout.write(txdata.data.dataAs<const char>(), txdata.data.size());
                            std::cout << "\'" << std::endl;
                        });
            })
            .onError([](Error&& err) {
                std::cerr << "Error: " << err.toString() << std::endl;
            });
}


int main(int argc, const char **argv) {

    P9Protocol::size_type bufferSize = P9Protocol::MAX_MESSAGE_SIZE;
    uint16 serverPort = 5640;
    StringView userName;
    StringView rootName("");
    StringView serverEndpoint("127.0.0.1");
    StringView dir("data");

    auto res = cli::Parser("libcadence/async_client", {
                            cli::Parser::printHelp(),
                            cli::Parser::printVersion("async_clent", cadence::getBuildVersion()),
                            {{"p", "port"}, "Server port", &serverPort},
                            {{"u", "user"}, "User name to Authenticate as", &userName},
                            {{"r", "root"}, "Resource root to attach to", &rootName},
                            {{"m", "msgSize"}, "Maximum message size", &bufferSize},
                            {{"h", "host"}, "Resource server endpoint", &serverEndpoint},
                            {{"d", "dir"}, "Directory to explore", &dir}})
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
    result
            .then([&]() {
                runTestSession(rootName, userName, dir, client);
            }).orElse([&ipEndpoint](Error&& er) {
                std::cout << "Failed to connect to '" << ipEndpoint << "': " << er.toString() << std::endl;
            });


    // Run event loop
    iocontext.run();

    return EXIT_SUCCESS;
}
