#include <arpa/inet.h>
#include <climits>
#include <glog/logging.h>
#include <utility>

#include "network_utils.h"

namespace lab1 {
    std::string NetworkUtils::getCurrentContainerHostname() {
        VLOG(1) << "getting current host";
        char hostname[HOST_NAME_MAX];
        int status = gethostname(hostname, HOST_NAME_MAX);
        CHECK(status == 0) << "cannot find the current host name, errorCode: " << errno;
        LOG(INFO) << "current host name: " << hostname;
        return std::string(hostname);
    }

    std::string NetworkUtils::getHostnameFromSocket(sockaddr *sockaddr) {
        char host[HOST_NAME_MAX + 1];
        int rv = getnameinfo(sockaddr, sizeof(*sockaddr), host, sizeof(host), nullptr, 0, 0);
        CHECK(rv == 0) << "could not get the name of the sender, error code: " << rv
                       << ", error message: " << gai_strerror(rv);
        return std::string(host);
    }

    const std::string HeartbeatSender::aliveMessage = "I am alive"; // NOLINT(cert-err58-cpp)
    const std::string HeartbeatSender::ackMessage = "Ack"; // NOLINT(cert-err58-cpp)
//    std::mutex HeartbeatSender::aliveMessageMutex;
//    std::unordered_set<std::string> HeartbeatSender::aliveMessageReceivers;
//    std::mutex HeartbeatSender::transportMutex;
//    std::unordered_map<std::string, std::shared_ptr<UDPSender>> HeartbeatSender::udpTransport;
//    std::atomic_bool HeartbeatSender::stopAliveMessageLoop = false;

//    void HeartbeatSender::startSendingAliveMessages() {
//        LOG(INFO) << "starting sending alive messages to peer";
//        while (!stopAliveMessageLoop) {
//            {
//                std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
//                LOG(INFO) << "sending alive messages to " << aliveMessageReceivers.size() << " receivers";
//                for (auto &hostname : aliveMessageReceivers) {
//                    auto udpSender = getUDPTransport(hostname);
//                    if (udpSender != nullptr) {
//                        udpSender->sendMessage(aliveMessage);
//                    } else {
//                        LOG(WARNING) << "UDPSender not found for host: " << hostname;
//                    }
//                }
//            }
//            LOG(INFO) << "sleeping in startSendingAliveMessages";
//            sleep(messageDelay);
//        }
//        for (auto &udpSender : udpTransport) {
//            udpSender.second->closeConnection();
//        }
//        LOG(INFO) << "stopped sending alive messages to peer";
//    }
//
//    void HeartbeatSender::addToAliveMessageReceiverList(const std::string &hostname) {
//        std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
//        LOG(INFO) << "adding " << hostname << " to aliveMessageReceivers";
//        aliveMessageReceivers.insert(hostname);
//    }
//
//    void HeartbeatSender::removeFromAliveMessageReceiverList(const std::string &hostname) {
//        std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
//        if (aliveMessageReceivers.erase(hostname) == 0) {
//            LOG(ERROR) << "cannot find UDPSender for host: " << hostname << " in broadcast list";
//        } else {
//            LOG(ERROR) << "removed host: " << hostname << " from broadcast list, queue size: "
//                       << aliveMessageReceivers.size();
//        }
//        if (aliveMessageReceivers.empty()) {
//            stopAliveMessageLoop = true;
//        }
//    }
//
//    int HeartbeatSender::getAliveMessageReceiverListSIze() {
//        std::lock_guard<std::mutex> lockGuard(aliveMessageMutex);
//        return aliveMessageReceivers.size();
//    }
//
//    void HeartbeatSender::sendingAckMessages(const std::string &hostname) {
//        LOG(INFO) << "sending ack message to host: " << hostname;
//        getUDPTransport(hostname)->sendMessage(ackMessage);
//        LOG(INFO) << "ack message sent to host: " << hostname;
//    }
//
//    std::shared_ptr<UDPSender> HeartbeatSender::getUDPTransport(const std::string &hostname) {
//        std::lock_guard<std::mutex> lockGuard(transportMutex);
//        if (udpTransport.find(hostname) == udpTransport.end()) {
//            try {
//                udpTransport.insert(
//                        std::make_pair(hostname, std::make_shared<UDPSender>(UDPSender(hostname, UDP_PORT))));
//                LOG(INFO) << "udpSenders size: " << udpTransport.size();
//            } catch (const std::runtime_error &e) {
//                LOG(ERROR) << "UDPSender creation error: " << e.what();
//            }
//        }
//
//        if (udpTransport.find(hostname) != udpTransport.end()) {
//            return udpTransport[hostname];
//        } else {
//            return nullptr;
//        }
//    }
//
//    HeartbeatReceiver::HeartbeatReceiver(UDPReceiver &udpReceiver, std::unordered_set<std::string> validSenders) :
//            udpReceiver(udpReceiver), validSenders(std::move(validSenders)) {}
//
//    void HeartbeatReceiver::startListeningForMessages() {
//        LOG(INFO) << "started listening for messages";
//        while (!validSenders.empty() || HeartbeatSender::getAliveMessageReceiverListSIze() != 0) {
//            auto pair = udpTransport.receiveMessage();
//            auto message = pair.first;
//            auto sender = pair.second;
//            // sender is of form <hostname>.<network-name>. e.g: "sumeet-g-alpha.cs7610-bridge"
//            // However the hostfile just contains "sumeet-g-alpha", hence need to split the sender string
//            sender = sender.substr(0, sender.find('.'));
//
//            if (message.rfind(HeartbeatSender::aliveMessage, 0) == 0) {
//                LOG(INFO) << "received alive message from hostname: " << sender;
//                HeartbeatSender::sendingAckMessages(sender);
//                validSenders.erase(sender);
//                LOG(INFO) << "removed sender: " << sender << ", from validSenders, validSendersSize: "
//                          << validSenders.size();
//            } else if (message.rfind(HeartbeatSender::ackMessage, 0) == 0) {
//                LOG(INFO) << "received ack from hostname: " << sender;
//                HeartbeatSender::removeFromAliveMessageReceiverList(sender);
//            } else {
//                LOG(ERROR) << "unknown message from: " << sender << ", message: " << message;
//            }
//        }
//        udpTransport.closeConnection();
//        LOG(INFO) << "stopped listening for messages, as received alive message from all peers";
//    }

    UDPTransport::UDPTransport(std::string hostname, int port) : hostname(std::move(hostname)),
                                                                 port(port), recvPort(port) {
        initReceiver();
        initSender();
    }

    void UDPTransport::initReceiver() {
        VLOG(1) << "initializing receiving socket for UDP transport on port: " << recvPort;
        struct addrinfo hints, *tempServerInfoList, *addrInfo;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP

        int rv = getaddrinfo(nullptr, std::to_string(recvPort).c_str(), &hints, &tempServerInfoList);
        CHECK(rv == 0) << "getaddrinfo failed: " << gai_strerror(rv);

        // loop through all the results and bind to the first we can
        for (addrInfo = tempServerInfoList;
             addrInfo != nullptr;
             addrInfo = addrInfo->ai_next) {
            if ((recvFD = socket(addrInfo->ai_family, addrInfo->ai_socktype,
                                 addrInfo->ai_protocol)) == -1) {
                LOG(ERROR) << "error in creating receiver socket";
                continue;
            }

            if (bind(recvFD, addrInfo->ai_addr, addrInfo->ai_addrlen) == -1) {
                ::close(recvFD);
                LOG(ERROR) << "error in binding the socket";
                continue;
            }

            LOG(INFO) << "receiver socket created";
            break;
        }

        CHECK(addrInfo != nullptr) << "failed to create receiver socket";
        freeaddrinfo(tempServerInfoList);
    }

    void UDPTransport::initSender() {
        VLOG(1) << "initializing sending socket for UDP transport for hostname: " << hostname << ", port:" << recvPort;
        addrinfo hints{};
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        int rv = getaddrinfo(hostname.c_str(), std::to_string(port).c_str(), &hints, &serverInfoList);
        CHECK(rv == 0) << "cannot find host: " << hostname << ", port: " << port
                       << ", getaddrinfo failed: " + std::string(gai_strerror(rv));

        // loop through all the results and make a socket
        for (serverAddrInfo = serverInfoList;
             serverAddrInfo != nullptr;
             serverAddrInfo = serverAddrInfo->ai_next) {
            if ((sendFD = socket(serverAddrInfo->ai_family, serverAddrInfo->ai_socktype,
                                 serverAddrInfo->ai_protocol)) == -1) {
                LOG(ERROR) << "error in creating sender socket";
                continue;
            }
            LOG(INFO) << "sender socket created for hostname: " << hostname << ", port:" << port;
            break;
        }
        CHECK(serverAddrInfo != nullptr) << "failed to create sender socket";
    }

    void UDPTransport::send(const std::string &message) {
        VLOG(1) << "sending to host: " << hostname << ":" << port << " ,message: " << message;
        if (int numbytes = sendto(sendFD, message.c_str(), message.size(), 0, serverAddrInfo->ai_addr,
                                  serverAddrInfo->ai_addrlen);
                numbytes == -1) {
            LOG(ERROR) << "error occurred while sending, host:" << hostname << ":" << port
                       << ", message: " << message;
        } else {
            LOG(INFO) << "sent to " << hostname << ":" << port << ", bytes: " << numbytes
                      << ", message: " << message;
        }
    }

    std::pair<std::string, std::string> UDPTransport::receive() {
        struct sockaddr_storage their_addr;
        socklen_t addr_len;
        addr_len = sizeof(their_addr);

        LOG(INFO) << "waiting for message";
        memset(&recvBuffer, 0, MAX_UDP_BUFFER_SIZE);
        if (int numbytes = recvfrom(recvFD, recvBuffer, MAX_UDP_BUFFER_SIZE - 1, 0,
                                    (struct sockaddr *) &their_addr, &addr_len);
                numbytes == -1) {
            std::string errorMessage("error(" + std::to_string(numbytes) + ") occurred while receiving data");
            LOG(ERROR) << errorMessage;
            throw std::runtime_error(errorMessage);
        } else {
            recvBuffer[numbytes] = '\0';
            std::string message(recvBuffer);
            std::string receivedFrom = NetworkUtils::getHostnameFromSocket((struct sockaddr *) &their_addr);
            LOG(INFO) << "received:" << numbytes << " bytes, from: " << receivedFrom << ", message: " << message;
            return std::pair<std::string, std::string>(message, receivedFrom);
        }
    }

    void UDPTransport::close() {
        LOG(INFO) << "closing UDPSender for host: " << hostname << ":" << port;
        freeaddrinfo(serverInfoList);
        ::close(sendFD);
    }
}
