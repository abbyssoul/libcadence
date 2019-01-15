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
#include <solace/output_utils.hpp>

#include <clime/parser.hpp>

#include <iostream>
#include <csignal>


using namespace Solace;
using namespace cadence;
using namespace cadence::async;


std::ostream& operator<< (std::ostream& ostr, NetworkEndpoint const& endpoint) {
    std::visit([](auto const& arg) {
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

    Session(Session&& rhs) noexcept
        : channel(std::move(rhs.channel))
        , remoteEndpoint(std::move(rhs.remoteEndpoint))
        , inBuffer{}
        , outBuffer{}
        , reader{wrapMemory(outBuffer)}
        , writer{wrapMemory(inBuffer)}
    {
        std::cout << "Moving Session" << NbSessions << std::endl;
        ++NbSessions;
    }


    void doRead() {
        if (channel.isClosed() || channel.getIOContext().isStopped()) {
            return;
        }

        auto self_ = shared_from_this();

        // Prepare in-buffer for the next read
        writer.clear();
        channel.asyncRead(writer)
            .then([self = std::move(self_)]() {
                // Encode data received
                auto encodedResponce = ByteWriter(wrapMemory(self->outBuffer));
                auto encoder = Base16Encoder(encodedResponce);
                encoder.encode(self->writer.viewWritten());

                // We add extra new-line to help with formatting
                encodedResponce.write('\n');

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
    auto serverEndpoint = StringView("127.0.0.1");

    auto res = clime::Parser("libcadence/async_server", {
                            clime::Parser::printHelp(),
                            clime::Parser::printVersion("async_server", cadence::getBuildVersion()),

                            {{"p", "port"}, "Server port", &serverPort},
                            {{"h", "host"}, "Server listen address", &serverEndpoint}
                           })
            .parse(argc, argv);

    if (!res) {
        auto const& e = res.getError();

        if (e) {
            std::cerr << "Error: " <<  e << std::endl;

            return EXIT_FAILURE;
        } else {
            std::cerr << e << std::endl;

            return EXIT_SUCCESS;
        }
    }

    auto ipParseResult = parseIPAddress(serverEndpoint);
    if (!ipParseResult) {
        std::cerr << "Error: " << ipParseResult.getError().toString() << std::endl;
        return EXIT_FAILURE;
    }

    EventLoop loop;
    auto const ipEndpoint = IPEndpoint(ipParseResult.unwrap(), serverPort);
    auto server = AsyncServer(loop, onNewConnection);
    server.startListen(ipEndpoint)
        .then([&ipEndpoint](){
            std::cout << "Server listening at " << ipEndpoint.toString() << std::endl;
        })
        .orElse([](Error&& e) {
            std::cerr << e.toString() << std::endl;
        });

    // Setup signal handlers for CTRL-C and SIGTERM
    auto sigs = SignalSet{loop, {SIGINT, SIGTERM}};
    sigs.asyncWait()
        .then([&server, &loop](int sig) {
            std::cout << "Signal: " << sig << ", stopping server" << std::endl;
            server.stop();
            loop.stop();

            std::cout << "Dangling sessions: " << Session::NbSessions << std::endl;
        });

    // Run event loop
    loop.run();

    return EXIT_SUCCESS;
}
