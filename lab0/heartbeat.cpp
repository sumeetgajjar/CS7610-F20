//
// Created by sumeet on 9/15/20.
//

#include <glog/logging.h>
#include <algorithm>
#include <utility>
#include <memory>
#include "socket_utils.h"
#include "heartbeat.h"
#include "config.h"

namespace lab0 {

    const std::string HeartbeatSender::aliveMessage = "I am alive"; // NOLINT(cert-err58-cpp)
    const std::string HeartbeatSender::ackMessage = "Ack"; // NOLINT(cert-err58-cpp)
    std::mutex HeartbeatSender::aliveMessageMutex;
    std::unordered_set<std::string> HeartbeatSender::aliveMessageReceivers;
    std::mutex HeartbeatSender::ackMessageMutex;
    std::unordered_set<std::string> HeartbeatSender::ackMessageReceivers;
    std::unordered_map<std::string, std::unique_ptr<UDPSender>> HeartbeatSender::udpSenders;
    std::atomic_bool HeartbeatSender::stopAliveMessageLoop = false, HeartbeatSender::stopAckMessageLoop = false;

    void HeartbeatSender::startSendingAliveMessages() {
        LOG(INFO) << "starting startSendingAliveMessages";
        while (!stopAliveMessageLoop) {
            {
                std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
                LOG(INFO) << "sending alive messages to " << aliveMessageReceivers.size() << " receivers";
                for (auto &hostname : aliveMessageReceivers) {
                    //TODO: resolve this
                    udpSenders[hostname].get()->sendMessage(aliveMessage);
                }
            }
            LOG(INFO) << "sleeping in startSendingAliveMessages";
            sleep(messageDelay);
        }
        LOG(INFO) << "stopping startSendingAliveMessages";
    }

    void HeartbeatSender::addToAliveMessageReceiverList(const std::string &hostname) {
        std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
        LOG(INFO) << "adding " << hostname << " to aliveMessageReceivers";
        udpSenders.insert(std::make_pair(hostname, std::make_unique<UDPSender>(UDPSender(hostname, UDP_PORT))));
        aliveMessageReceivers.insert(hostname);
    }

    void HeartbeatSender::removeFromAliveMessageReceiverList(const std::string &hostname) {
        std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
        if (aliveMessageReceivers.erase(hostname) == 0) {
            LOG(ERROR) << "Cannot find UDPSender for host: " << hostname << " in broadcast list";
        } else {
            LOG(ERROR) << "Removed host: " << hostname << " from broadcast list";
        }
        if (aliveMessageReceivers.empty()) {
            stopAliveMessageLoop = true;
        }
    }

    void HeartbeatSender::sendingAckMessages() {
        // TODO: only send once
        while (!stopAckMessageLoop) {
            {
                std::lock_guard<std::mutex> lockGuard(ackMessageMutex);
                for (auto &hostname : ackMessageReceivers) {
                    udpSenders[hostname].get()->sendMessage(ackMessage);
                }
            }
            sleep(messageDelay);
        }
    }

    void HeartbeatSender::addToAckMessageReceiverList(const std::string &hostname) {
        std::lock_guard<std::mutex> lockGuard(ackMessageMutex);
        ackMessageReceivers.insert(hostname);
    }

    void HeartbeatSender::removeFromAckMessageReceiverList(const std::string &hostname) {
        std::lock_guard<std::mutex> lockGuard(ackMessageMutex);
        if (ackMessageReceivers.erase(hostname) == 0) {
            LOG(ERROR) << "Cannot find UDPSender for host: " << hostname << " in broadcast list";
        } else {
            LOG(ERROR) << "Removed host: " << hostname << " from broadcast list";
        }
        if (ackMessageReceivers.empty()) {
            stopAckMessageLoop = true;
        }
    }

    HeartbeatReceiver::HeartbeatReceiver(UDPReceiver &udpReceiver) : udpReceiver(udpReceiver) {}

    void HeartbeatReceiver::startListeningForMessages() {
        while (true) {
            // TODO: think about this
            auto pair = udpReceiver.receiveMessage();
            std::string message = pair.first;
            std::string sender = pair.second;

            if (message.rfind(HeartbeatSender::aliveMessage, 0) == 0) {
                HeartbeatSender::addToAckMessageReceiverList(sender);
                HeartbeatSender::removeFromAliveMessageReceiverList(sender);
            } else if (message.rfind(HeartbeatSender::ackMessage, 0) == 0) {
                HeartbeatSender::removeFromAckMessageReceiverList(sender);
            }
        }
    }
}