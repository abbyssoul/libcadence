/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/

#include <cadence/asyncServer.hpp>
#include <cadence/networkEndpoint.hpp>
#include <cadence/version.hpp>
#include <cadence/async/signalSet.hpp>

#include <solace/memoryManager.hpp>
#include <solace/uuid.hpp>
#include <solace/base16.hpp>
#include <solace/cli/parser.hpp>
#include <solace/output_utils.hpp>


#include <iostream>
#include <csignal>


using namespace Solace;
using namespace cadence;
using namespace cadence::async;


std::ostream& operator<< (std::ostream& ostr, NetworkEndpoint const& endpoint) {
    std::visit([](auto&& arg) {
        std::cout << arg.toString();
    }, endpoint);

    return ostr;
}


class Session
        : public std::enable_shared_from_this<Session> {
public:

    static uint32 NbSessions;

    ~Session() {
        std::cout << "Session from " << remoteEndpoint << " closed" << std::endl;
        --NbSessions;
    }

    Session(async::StreamSocket&& c)
        : channel(std::move(c))
        , remoteEndpoint(channel.getRemoteEndpoint())
    {
        ++NbSessions;
    }

    Session(Session&& rhs)
        : channel(std::move(rhs.channel))
        , remoteEndpoint(std::move(rhs.remoteEndpoint))
        , inBuffer{}
        , outBuffer{}
        , reader{wrapMemory(outBuffer)}
        , writer{wrapMemory(inBuffer)}
    {
        std::cout << "Moving Session" << NbSessions << std::endl;
    }


    void doRead() {
        if (channel.getIOContext().isStopped())
            return;

        auto self_ = shared_from_this();

        // Prepare in-buffer for the next read
        writer.clear();
        channel.asyncRead(writer)
                .then([self = std::move(self_)]() {
//                    std::cout << "Data from [" << self->remoteEndpoint << "]: "
//                              << "\"" << self->writer.viewWritten() << "\"" << std::endl;

                    // Encode data received
                    ByteWriter encodedResp(wrapMemory(self->outBuffer));
                    Base16Encoder encoder(encodedResp);
                    encoder.encode(self->writer.viewWritten());
                    encodedResp.write('\n');

                    self->channel.asyncWrite(self->reader)
                            .then([self_cont = std::move(self)]() {
                                self_cont->reader.rewind();
                                self_cont->doRead();
                            })
                            .onError([](Error&& e) {
                                std::cerr << "Failed to write respoce: " << e.toString() << std::endl;
                            });
                });
    }

private:
    async::StreamSocket channel;
    NetworkEndpoint remoteEndpoint;

    byte inBuffer[10];
    byte outBuffer[21];
    ByteReader reader{wrapMemory(outBuffer)};
    ByteWriter writer{wrapMemory(inBuffer)};
};


void onNewConnection(async::StreamSocket&& channel) {
    std::cout << "New connection from " << channel.getRemoteEndpoint() << std::endl;

    auto session = std::make_shared<Session>(std::move(channel));
    session->doRead();
}


uint32 Session::NbSessions = 0;

int main(int argc, const char **argv) {
    uint16 serverPort = 5640;
    StringView serverEndpoint("127.0.0.1");

    auto res = cli::Parser("libcadence/async_server", {
                            cli::Parser::printHelp(),
                            cli::Parser::printVersion("async_server", cadence::getBuildVersion()),

                            {{"p", "port"}, "Server port", &serverPort},
                            {{"h", "host"}, "Server listen address", &serverEndpoint}
                           })
            .parse(argc, argv);

    if (!res) {
        const auto& e = res.getError();

        if (e) {
            std::cerr << "Error: " <<  e.toString() << std::endl;

            return EXIT_FAILURE;
        } else {
            std::cerr << e.toString() << std::endl;

            return EXIT_SUCCESS;
        }
    }

    auto ipParseResult = parseIPAddress(serverEndpoint);
    if (!ipParseResult) {
        std::cerr << "Error: " << ipParseResult.getError().toString() << std::endl;
        return EXIT_FAILURE;
    }

    IPAddress const serverIp = ipParseResult.unwrap();
    EventLoop iocontext;

    AsyncServer server(iocontext, onNewConnection);

    IPEndpoint ipEndpoint(serverIp, serverPort);
    server.startListen(ipEndpoint)
            .then([&ipEndpoint](){
                std::cout << "Server listening at " << ipEndpoint.toString() << std::endl;
            })
            .orElse([](Error&& e) {
                std::cerr << e.toString() << std::endl;
            });

    // Setup signal handlers for CTRL-C and SIGTERM
    auto sigs = SignalSet{iocontext, {SIGINT, SIGTERM}};
    sigs.asyncWait()
            .then([&server, &iocontext](int sig) {
                std::cout << "Signal: " << sig << ", stopping server" << std::endl;
                server.stop();
                iocontext.stop();

                std::cout << "Dangling sessions: " << Session::NbSessions << std::endl;
            });

    // Run event loop
    iocontext.run();

    return EXIT_SUCCESS;
}
