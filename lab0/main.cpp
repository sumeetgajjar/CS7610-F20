#include <iostream>
#include <glog/logging.h>
#include <thread>

#include "utils.h"
#include "socket_utils.h"


using namespace lab0;

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    LOG(INFO) << "Testing logging";
    const auto hostFilePath = Utils::parseHostFileFromCmdArguments(argc, argv);
    const auto hostnames = Utils::readHostFile(hostFilePath);
    const auto currentContainerHostname = Utils::getCurrentContainerHostname();
    const auto peerContainerHostnames = Utils::getPeerContainerHostnames(hostnames, currentContainerHostname);
    const auto currentProcessIdentifier = Utils::getProcessIdentifier(hostnames, currentContainerHostname);

    std::cout << currentContainerHostname << " -> Process Number -> " << currentProcessIdentifier << std::endl;

    std::thread r([]() {
        Receiver receiver(1234);
        auto pair = receiver.receiveMessage();
        receiver.closeConnection();
    });
    sleep(2);
    std::thread s([]() {
        Sender sender("localhost", 1234);
        sender.sendMessage("Hello from sender");
        sender.closeConnection();
    });
    s.join();
    r.join();

    return 0;
}
