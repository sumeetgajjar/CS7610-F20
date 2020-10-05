//
// Created by sumeet on 10/2/20.
//

#ifndef LAB1_PROBE_UTILS_H
#define LAB1_PROBE_UTILS_H

#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <atomic>
#include "network_utils.h"

#define PROBING_PORT 10000

namespace lab1 {

    class ProbeSender {
    private:
        static std::mutex aliveMessageMutex;
        static std::unordered_set<std::string> aliveMessageReceivers;
        static std::mutex udpSendersMutex;
        static std::unordered_map<std::string, std::shared_ptr<UDPSender>> udpSenders;
        static std::atomic_bool stopAliveMessageLoop;

        static const int messageDelay = 4;
    public:

        static std::shared_ptr<UDPSender> getUDPSender(const std::string &hostname);

        static const std::string aliveMessage;
        static const std::string ackMessage;

        static void startSendingAliveMessages();

        static void addToAliveMessageReceiverList(const std::string &hostname);

        static void removeFromAliveMessageReceiverList(const std::string &hostname);

        static int getAliveMessageReceiverListSize();

        static void sendingAckMessages(const std::string &hostname);

    };

    class ProbeReceiver {
    private:
        UDPReceiver udpReceiver;
        std::unordered_set<std::string> validSenders;
    public:
        explicit ProbeReceiver(const std::vector<std::string> &peerHostnames);

        void startListeningForMessages();
    };

    class ProbingUtils {
    public:
        static void waitForPeersToStart(const std::vector<std::string> &peerHostnames);
    };
}

#endif //LAB1_PROBE_UTILS_H
