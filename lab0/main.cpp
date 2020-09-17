#include <iostream>
#include <glog/logging.h>
#include <thread>

#include "config.h"
#include "utils.h"
#include "heartbeat.h"
#include "socket_utils.h"


using namespace lab0;

void testUDPSenderAndReceiver() {
    std::thread r([]() {
        UDPReceiver receiver(1234);
        auto pair = receiver.receiveMessage();
        receiver.closeConnection();
    });
    sleep(2);
    std::thread s([]() {
        UDPSender sender("localhost", 1234);
        sender.sendMessage("Hello from sender");
        sender.closeConnection();
    });
    s.join();
    r.join();
}

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    const auto hostFilePath = Utils::parseHostFileFromCmdArguments(argc, argv);
    const auto hostnames = Utils::readHostFile(hostFilePath);
    const auto currentContainerHostname = Utils::getCurrentContainerHostname();
    const auto peerContainerHostnames = Utils::getPeerContainerHostnames(hostnames, currentContainerHostname);
    const auto currentProcessIdentifier = Utils::getProcessIdentifier(hostnames, currentContainerHostname);

    for (const auto &peerContainerHostname : peerContainerHostnames) {
        HeartbeatSender::addToAliveMessageReceiverList(peerContainerHostname);
    }

    std::thread alive([]() {
        HeartbeatSender::startSendingAliveMessages();
    });

    std::thread receive([&]() {
        UDPReceiver udpReceiver(UDP_PORT);
        std::unordered_set<std::string> validSenders(peerContainerHostnames.begin(), peerContainerHostnames.end());
        HeartbeatReceiver heartbeatReceiver(udpReceiver, validSenders);
        heartbeatReceiver.startListeningForMessages();
    });

    alive.join();
    receive.join();
    LOG(INFO) << "READY";

    return 0;
}
