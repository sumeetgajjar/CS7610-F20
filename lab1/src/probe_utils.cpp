//
// Created by sumeet on 10/2/20.
//
#include <glog/logging.h>
#include <condition_variable>
#include <thread>

#include "probe_utils.h"
#include "utils.h"

namespace lab1 {
    const std::string ProbeSender::aliveMessage = "I am alive"; // NOLINT(cert-err58-cpp)

    const std::string ProbeSender::ackMessage = "Ack"; // NOLINT(cert-err58-cpp)
    std::mutex ProbeSender::aliveMessageMutex;
    std::unordered_set<std::string> ProbeSender::aliveMessageReceivers;
    std::mutex ProbeSender::udpSendersMutex;
    std::unordered_map<std::string, std::shared_ptr<UDPSender>> ProbeSender::udpSenders;
    std::atomic_bool ProbeSender::stopAliveMessageLoop = false;

    void ProbeSender::startSendingAliveMessages() {
        LOG(INFO) << "starting sending alive messages to peer";
        while (!stopAliveMessageLoop) {
            {
                std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
                LOG(INFO) << "sending alive messages to " << aliveMessageReceivers.size() << " receivers";
                for (auto &hostname : aliveMessageReceivers) {
                    auto udpSender = getUDPSender(hostname);
                    if (udpSender != nullptr) {
                        udpSender->send(aliveMessage);
                    } else {
                        LOG(WARNING) << "UDPSender not found for host: " << hostname;
                    }
                }
            }
            LOG(INFO) << "sleeping in startSendingAliveMessages";
            sleep(messageDelay);
        }
        for (auto &udpSender : udpSenders) {
            udpSender.second->close();
        }
        LOG(INFO) << "stopped sending alive messages to peer";
    }

    void ProbeSender::addToAliveMessageReceiverList(const std::string &hostname) {
        std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
        LOG(INFO) << "adding " << hostname << " to aliveMessageReceivers";
        aliveMessageReceivers.insert(hostname);
    }

    void ProbeSender::removeFromAliveMessageReceiverList(const std::string &hostname) {
        std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
        if (aliveMessageReceivers.erase(hostname) == 0) {
            LOG(WARNING) << "cannot find UDPSender for host: " << hostname << " in broadcast list";
        } else {
            LOG(ERROR) << "removed host: " << hostname << " from broadcast list, queue size: "
                       << aliveMessageReceivers.size();
        }
        if (aliveMessageReceivers.empty()) {
            stopAliveMessageLoop = true;
        }
    }

    int ProbeSender::getAliveMessageReceiverListSize() {
        VLOG(1) << "inside getAliveMessageReceiverListSize of ProbeSender";
        std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
        VLOG(1) << "aliveMessageReceivers size: " << aliveMessageReceivers.size();
        return aliveMessageReceivers.size();
    }

    void ProbeSender::sendingAckMessages(const std::string &hostname) {
        LOG(INFO) << "sending ack message to host: " << hostname;
        getUDPSender(hostname)->send(ackMessage);
        LOG(INFO) << "ack message sent to host: " << hostname;
    }

    std::shared_ptr<UDPSender> ProbeSender::getUDPSender(const std::string &hostname) {
        VLOG(1) << "inside getUDPSender of ProbeSender";
        std::lock_guard<std::mutex> lockGuard(udpSendersMutex);
        if (udpSenders.find(hostname) == udpSenders.end()) {
            try {
                udpSenders.insert(std::make_pair(hostname, std::make_shared<UDPSender>(UDPSender(hostname,
                                                                                                 PROBING_PORT))));
                LOG(INFO) << "udpSenders size: " << udpSenders.size();
            } catch (const std::runtime_error &e) {
                LOG(ERROR) << "UDPSender creation error: " << e.what();
            }
        }

        if (udpSenders.find(hostname) != udpSenders.end()) {
            return udpSenders.at(hostname);
        } else {
            return nullptr;
        }
    }

    ProbeReceiver::ProbeReceiver(const std::vector<std::string> &peerHostnames) : udpReceiver(PROBING_PORT) {
        this->validSenders.insert(peerHostnames.begin(), peerHostnames.end());
    }

    void ProbeReceiver::startListeningForMessages() {
        LOG(INFO) << "started listening for messages";
        while (!validSenders.empty() || ProbeSender::getAliveMessageReceiverListSize() != 0) {
            auto message = udpReceiver.receive();
            auto messageString = std::string(message.buffer, message.n);
            auto sender = Utils::parseHostnameFromSender(message.sender);
            // sender is of form <hostname>.<network-name>. e.g: "sumeet-g-alpha.cs7610-bridge"
            // However the hostfile just contains "sumeet-g-alpha", hence need to split the sender string
            sender = sender.substr(0, sender.find('.'));

            if (messageString.rfind(ProbeSender::aliveMessage, 0) == 0) {
                LOG(INFO) << "received alive message from hostname: " << sender;
                ProbeSender::sendingAckMessages(sender);
                validSenders.erase(sender);
                LOG(INFO) << "removed sender: " << sender << ", from validSenders, validSendersSize: "
                          << validSenders.size();
            } else if (messageString.rfind(ProbeSender::ackMessage, 0) == 0) {
                LOG(INFO) << "received ack from hostname: " << sender;
                ProbeSender::removeFromAliveMessageReceiverList(sender);
            } else {
                LOG(ERROR) << "unknown message from: " << sender << ", message: " << messageString;
            }
        }
        udpReceiver.close();
        LOG(INFO) << "stopped listening for messages, as received alive message from all peers";
    }

    void ProbingUtils::waitForPeersToStart(const std::vector<std::string> &peerHostnames) {
        std::mutex mutex;
        std::condition_variable conditionVariable;
        bool heartbeatReceiverReady = false;

        std::thread receive([&]() {
            ProbeReceiver probeReceiver(peerHostnames);
            // Signaling HeartbeatSender to start sending alive messages
            {
                std::lock_guard<std::mutex> lockGuard(mutex);
                heartbeatReceiverReady = true;
            }
            conditionVariable.notify_one();
            probeReceiver.startListeningForMessages();
        });

        std::thread alive([&]() {
            for (const auto &peerContainerHostname : peerHostnames) {
                ProbeSender::addToAliveMessageReceiverList(peerContainerHostname);
            }
            // Waiting for HeartBeastReceiver to startListeningForMessages
            std::unique_lock<std::mutex> uniqueLock(mutex);
            conditionVariable.wait(uniqueLock, [&] { return heartbeatReceiverReady; });
            ProbeSender::startSendingAliveMessages();
        });

        receive.join();
        alive.join();
        LOG(INFO) << "all peers are ready";
    }
}