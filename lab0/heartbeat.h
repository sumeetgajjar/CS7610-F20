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
        static std::mutex ackMessageMutex;
        static std::unordered_set<std::string> ackMessageReceivers;
        static std::unordered_map<std::string, std::unique_ptr<UDPSender>> udpSenders;
        static std::atomic_bool stopAliveMessageLoop, stopAckMessageLoop;
        static const int messageDelay = 2;
    public:
        static const std::string aliveMessage;
        static const std::string ackMessage;

        static void startSendingAliveMessages();

        static void addAliveReceiverList(const std::string &hostname);

        static void removeFromAliveReceiverList(const std::string &hostname);

        static void startSendingAckMessages();

        static void addToAckReceiverList(const std::string &hostname);

        static void removeFromAckReceiverList(const std::string &hostname);
    };

    class HeartbeatReceiver {
    private:
        UDPReceiver udpReceiver;
    public:
        HeartbeatReceiver(UDPReceiver &udpReceiver);

        void startListeningForMessages();
    };
}


#endif //LAB0_HEARTBEAT_H
