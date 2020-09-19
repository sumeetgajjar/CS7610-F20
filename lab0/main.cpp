#include <iostream>
#include <glog/logging.h>
#include <thread>
#include <condition_variable>

#include "config.h"
#include "utils.h"
#include "heartbeat.h"
#include "socket_utils.h"


using namespace lab0;

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    const auto hostFilePath = Utils::parseHostFileFromCmdArguments(argc, argv);
    const auto hostnames = Utils::readHostFile(hostFilePath);
    const auto currentContainerHostname = Utils::getCurrentContainerHostname();
    const auto peerContainerHostnames = Utils::getPeerContainerHostnames(hostnames, currentContainerHostname);
    const auto currentProcessIdentifier = Utils::getProcessIdentifier(hostnames, currentContainerHostname);

    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool heartbeatReceiverReady = false;

    std::thread receive([&]() {
        UDPReceiver udpReceiver(UDP_PORT);
        std::unordered_set<std::string> validSenders(peerContainerHostnames.begin(), peerContainerHostnames.end());
        HeartbeatReceiver heartbeatReceiver(udpReceiver, validSenders);

        // Signaling HeartbeatSender to start sending alive messages
        {
            std::lock_guard<std::mutex> lockGuard(mutex);
            heartbeatReceiverReady = true;
        }
        conditionVariable.notify_one();

        heartbeatReceiver.startListeningForMessages();
    });

    std::thread alive([&]() {
        for (const auto &peerContainerHostname : peerContainerHostnames) {
            HeartbeatSender::addToAliveMessageReceiverList(peerContainerHostname);
        }

        // Waiting for HeartBeastReceiver to startListeningForMessages
        std::unique_lock<std::mutex> uniqueLock(mutex);
        conditionVariable.wait(uniqueLock, [&] { return heartbeatReceiverReady; });

        HeartbeatSender::startSendingAliveMessages();
    });

    receive.join();
    alive.join();
    LOG(INFO) << "READY";

    return 0;
}
