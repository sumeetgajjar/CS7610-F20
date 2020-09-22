//
// Created by sumeet on 9/15/20.
//

#ifndef LAB0_HEARTBEAT_H
#define LAB0_HEARTBEAT_H

#include <vector>
#include <mutex>
#include <utility>
#include <unordered_map>
#include <atomic>
#include <unordered_set>
#include "socket_utils.h"

namespace lab0 {
    class HeartbeatSender {
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

        static int getAliveMessageReceiverListSIze();

        static void sendingAckMessages(const std::string &hostname);

    };

    class HeartbeatReceiver {
    private:
        UDPReceiver udpReceiver;
        std::unordered_set<std::string> validSenders;
    public:
        HeartbeatReceiver(UDPReceiver &udpReceiver, std::unordered_set<std::string> validSenders);

        void startListeningForMessages();
    };
}


#endif //LAB0_HEARTBEAT_H
