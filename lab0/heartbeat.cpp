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
#include "utils.h"

namespace lab0 {

    const std::string HeartbeatSender::aliveMessage = "I am alive"; // NOLINT(cert-err58-cpp)
    const std::string HeartbeatSender::ackMessage = "Ack"; // NOLINT(cert-err58-cpp)
    std::mutex HeartbeatSender::aliveMessageMutex;
    std::unordered_set<std::string> HeartbeatSender::aliveMessageReceivers;
    std::mutex HeartbeatSender::udpSendersMutex;
    std::unordered_map<std::string, std::shared_ptr<UDPSender>> HeartbeatSender::udpSenders;
    std::atomic_bool HeartbeatSender::stopAliveMessageLoop = false;

    void HeartbeatSender::startSendingAliveMessages() {
        LOG(INFO) << "starting sending alive messages to peer";
        while (!stopAliveMessageLoop) {
            {
                std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
                LOG(INFO) << "sending alive messages to " << aliveMessageReceivers.size() << " receivers";
                for (auto &hostname : aliveMessageReceivers) {
                    auto udpSender = getUDPSender(hostname);
                    if (udpSender != nullptr) {
                        udpSender->sendMessage(aliveMessage);
                    } else {
                        LOG(WARNING) << "UDPSender not found for host: " << hostname;
                    }
                }
            }
            LOG(INFO) << "sleeping in startSendingAliveMessages";
            sleep(messageDelay);
        }
        LOG(INFO) << "stopped sending alive messages to peer";
    }

    void HeartbeatSender::addToAliveMessageReceiverList(const std::string &hostname) {
        std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
        LOG(INFO) << "adding " << hostname << " to aliveMessageReceivers";
        aliveMessageReceivers.insert(hostname);
    }

    void HeartbeatSender::removeFromAliveMessageReceiverList(const std::string &hostname) {
        std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
        if (aliveMessageReceivers.erase(hostname) == 0) {
            LOG(ERROR) << "cannot find UDPSender for host: " << hostname << " in broadcast list";
        } else {
            LOG(ERROR) << "removed host: " << hostname << " from broadcast list, queue size: "
                       << aliveMessageReceivers.size();
        }
        if (aliveMessageReceivers.empty()) {
            stopAliveMessageLoop = true;
        }
    }

    void HeartbeatSender::sendingAckMessages(const std::string &hostname) {
        LOG(INFO) << "sending ack message to host: " << hostname;
        getUDPSender(hostname)->sendMessage(ackMessage);
        LOG(INFO) << "ack message sent to host: " << hostname;
    }

    std::shared_ptr<UDPSender> HeartbeatSender::getUDPSender(const std::string &hostname) {
        std::lock_guard<std::mutex> lockGuard(udpSendersMutex);
        if (udpSenders.find(hostname) == udpSenders.end()) {
            try {
                udpSenders.insert(std::make_pair(hostname, std::make_shared<UDPSender>(UDPSender(hostname, UDP_PORT))));
                LOG(INFO) << "udpSenders size: " << udpSenders.size();
            } catch (const std::runtime_error &e) {
                LOG(ERROR) << "UDPSender creation error: " << e.what();
            }
        }

        if (udpSenders.find(hostname) != udpSenders.end()) {
            return udpSenders[hostname];
        } else {
            return nullptr;
        }
    }

    int HeartbeatSender::getAliveMessageReceiverListSIze() {
        std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
        return aliveMessageReceivers.size();
    }

    HeartbeatReceiver::HeartbeatReceiver(UDPReceiver &udpReceiver, std::unordered_set<std::string> validSenders) :
            udpReceiver(udpReceiver), validSenders(std::move(validSenders)) {}

    void HeartbeatReceiver::startListeningForMessages() {
        LOG(INFO) << "started listening for messages";
        while (!validSenders.empty() || HeartbeatSender::getAliveMessageReceiverListSIze() != 0) {
            auto pair = udpReceiver.receiveMessage();
            auto message = pair.first;
            auto sender = pair.second;
            // sender is of form <hostname>.<network-name>. e.g: "sumeet-g-alpha.cs7610-bridge"
            // However the hostfile just contains "sumeet-g-alpha", hence need to split the sender string
            sender = sender.substr(0, sender.find('.'));

            if (message.rfind(HeartbeatSender::aliveMessage, 0) == 0) {
                LOG(INFO) << "received alive message from hostname: " << sender;
                HeartbeatSender::sendingAckMessages(sender);
                validSenders.erase(sender);
                LOG(INFO) << "removed sender: " << sender << ", from validSenders";
            } else if (message.rfind(HeartbeatSender::ackMessage, 0) == 0) {
                LOG(INFO) << "received ack from hostname: " << sender;
                HeartbeatSender::removeFromAliveMessageReceiverList(sender);
            } else {
                LOG(ERROR) << "unknown message from: " << sender << ", message: " << message;
            }
        }
        LOG(INFO) << "stopped listening for messages, as received alive message from all peers";
    }
}