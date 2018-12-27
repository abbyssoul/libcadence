/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/

#include <cadence/async/serial.hpp>
#include <cadence/version.hpp>

#include <solace/memoryManager.hpp>
#include <solace/cli/parser.hpp>
#include <solace/output_utils.hpp>


#include <iostream>

using namespace Solace;
using namespace cadence::async;


void enumerateDevices() {
    for (const auto& descriptor : Solace::IO::Serial::enumeratePorts()) {
        std::cout << descriptor.file << ":" << std::endl;
        std::cout << "\t - " << descriptor.description << std::endl;
        std::cout << "\t - " << descriptor.hardwareId << std::endl;
    }
}


int main(int argc, const char **argv) {
    if (argc < 2) {  // No-arg call: list ports and exit
        enumerateDevices();
        return 0;
    }


    uint32 boudRate = 115200;
    uint32 bufferSize = 120;
    StringView devPath;

    auto res = cli::Parser("libcadence/async_serial", {
                          cli::Parser::printHelp(),
                          cli::Parser::printVersion("async_serial", cadence::getBuildVersion()),
                          {{"b", "boud"}, "Boud rate", &boudRate},
                          {{"bufferSize"}, "Read buffer size", &bufferSize},
                          {{"f", "file"}, "Path to the serial device", &devPath}})
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


    EventLoop iocontext;
    SerialChannel serial(iocontext, Path::parse(devPath).unwrap(), boudRate);

    MemoryManager memManager(2048);
    ByteWriter readBuffer(memManager.allocate(bufferSize));

    serial.asyncRead(readBuffer)
            .then([&readBuffer]() {
                auto dataView = readBuffer.viewWritten();
                std::cout.write(dataView.dataAs<const char>(), dataView.size());
                std::cout.flush();

                readBuffer.rewind();
            });

    iocontext.run();

    return EXIT_SUCCESS;
}
