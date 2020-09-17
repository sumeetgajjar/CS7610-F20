//
// Created by sumeet on 9/14/20.
//

#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <glog/logging.h>
#include <climits>
#include <utility>

#include "socket_utils.h"

namespace lab0 {

    UDPSender::UDPSender(std::string host, int port) : serverHost(std::move(host)), serverPort(std::to_string(port)) {
        LOG(INFO) << "creating UDPSender for hostname: " << serverHost << ", port: " << serverPort;
        initSocket();
    };

    void UDPSender::initSocket() {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

        if (int rv = getaddrinfo(serverHost.c_str(), serverPort.c_str(), &hints, &serverInfoList);
                rv != 0) {
            throw std::runtime_error("cannot find host: " + serverHost + ", port: " + serverPort +
                                     ", getaddrinfo failed: " + std::string(gai_strerror(rv)));
        }

        // loop through all the results and make a socket
        for (serverAddrInfo = serverInfoList; serverAddrInfo != NULL; serverAddrInfo = serverAddrInfo->ai_next) {
            if ((socketFD = socket(serverAddrInfo->ai_family, serverAddrInfo->ai_socktype,
                                   serverAddrInfo->ai_protocol)) == -1) {
                LOG(ERROR) << "error in creating sender socket";
                continue;
            }
            LOG(INFO) << "sender socket created for hostname: " << serverHost << ", port:" << serverPort;
            break;
        }

        if (serverAddrInfo == NULL) {
            LOG(FATAL) << "failed to create sender socket";
        }
    }

    void UDPSender::sendMessage(const std::string &message) {
        LOG(INFO) << "sending to host: " << serverHost << ":" << serverPort << " ,message: " << message;
        if (int numbytes = sendto(socketFD, message.c_str(), message.size(), 0, serverAddrInfo->ai_addr,
                                  serverAddrInfo->ai_addrlen);
                numbytes == -1) {
            LOG(ERROR) << "error occurred while sending, host:" << serverHost << ":" << serverPort
                       << ", message: " << message;
        } else {
            LOG(INFO) << "sent to " << serverHost << ":" << serverPort << ", bytes: " << numbytes
                      << ", message: " << message;
        }
    }

    void UDPSender::closeConnection() {
        LOG(INFO) << "closing UDPSender for host: " << serverHost << ":" << serverPort;
        freeaddrinfo(serverInfoList);
        close(socketFD);
    }

    void UDPReceiver::initSocket() {
        struct addrinfo hints, *serverInfoList, *serverAddrInfo;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP

        if (int rv = getaddrinfo(NULL, serverPort.c_str(), &hints, &serverInfoList);
                rv != 0) {
            LOG(FATAL) << "getaddrinfo failed: " << gai_strerror(rv);
        }

        // loop through all the results and bind to the first we can
        for (serverAddrInfo = serverInfoList; serverAddrInfo != NULL; serverAddrInfo = serverAddrInfo->ai_next) {
            if ((sockfd = socket(serverAddrInfo->ai_family, serverAddrInfo->ai_socktype,
                                 serverAddrInfo->ai_protocol)) == -1) {
                LOG(ERROR) << "error in creating receiver socket";
                continue;
            }

            if (bind(sockfd, serverAddrInfo->ai_addr, serverAddrInfo->ai_addrlen) == -1) {
                close(sockfd);
                LOG(ERROR) << "error in binding the socket";
                continue;
            }

            LOG(INFO) << "receiver socket created";
            break;
        }

        if (serverAddrInfo == NULL) {
            LOG(FATAL) << "failed to create receiver socket";
        }

        freeaddrinfo(serverInfoList);
    }

    UDPReceiver::UDPReceiver(int port) : serverPort(std::to_string(port)) {
        initSocket();
    }

    std::pair<std::string, std::string> UDPReceiver::receiveMessage() {
        struct sockaddr_storage their_addr;
        socklen_t addr_len;
        addr_len = sizeof(their_addr);

        LOG(INFO) << "waiting for message";
        memset(&buffer, 0, MAX_BUFFER_SIZE);
        if (int numbytes = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE - 1, 0, (struct sockaddr *) &their_addr,
                                    &addr_len);
                numbytes == -1) {
            std::string errorMessage("error(" + std::to_string(numbytes) + ") occurred while receiving data");
            LOG(ERROR) << errorMessage;
            throw std::runtime_error(errorMessage);
        } else {
            buffer[numbytes] = '\0';
            std::string message(buffer);
            std::string receivedFrom = SocketUtils::getHostnameFromSocket((struct sockaddr *) &their_addr);
            LOG(INFO) << "received:" << numbytes << " bytes, from: " << receivedFrom << ", message: " << message;
            return std::pair<std::string, std::string>(message, receivedFrom);
        }
    }

    void UDPReceiver::closeConnection() {
        LOG(INFO) << "closing UDPReceiver for host";
        close(sockfd);
    }

    std::string SocketUtils::getHostnameFromSocket(sockaddr *sockaddr) {
        char host[HOST_NAME_MAX + 1];
        if (int rv = getnameinfo(sockaddr, sizeof(*sockaddr), host, sizeof(host), NULL, 0, 0);
                rv != 0) {
            LOG(FATAL) << "could not get the name of the sender, error code: " << rv << ", error message: "
                       << gai_strerror(rv);
        }
        return std::string(host);
    }
}
