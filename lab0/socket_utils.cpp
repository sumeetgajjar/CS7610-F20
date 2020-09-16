//
// Created by sumeet on 9/14/20.
//

#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <glog/logging.h>
#include <climits>

#include "socket_utils.h"

namespace lab0 {


    void UDPSender::initSocket() {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

        if (int rv = getaddrinfo(serverHost.c_str(), serverPort.c_str(), &hints, &serverInfoList);
                rv != 0) {
            LOG(FATAL) << "getaddrinfo failed: " << gai_strerror(rv);
        }

        // loop through all the results and make a socket
        for (serverAddrInfo = serverInfoList; serverAddrInfo != NULL; serverAddrInfo = serverAddrInfo->ai_next) {
            if ((socketFD = socket(serverAddrInfo->ai_family, serverAddrInfo->ai_socktype,
                                   serverAddrInfo->ai_protocol)) == -1) {
                LOG(ERROR) << "error in creating sender socket";
                continue;
            }
            LOG(INFO) << "sender socket created";
            break;
        }

        if (serverAddrInfo == NULL) {
            LOG(FATAL) << "failed to create sender socket";
        }
    }

    void UDPSender::sendMessage(const std::string &message) {
        LOG(INFO) << "sending message: " << message;
        if (int numbytes = sendto(socketFD, message.c_str(), message.size(), 0, serverAddrInfo->ai_addr,
                                  serverAddrInfo->ai_addrlen);
                numbytes == -1) {
            LOG(ERROR) << "error(" << numbytes << ") occurred while sending message: " << message;
        } else {
            LOG(INFO) << "sent to " << serverHost << ", bytes: " << numbytes << ", message: " << message;
        }
    }

    void UDPSender::closeConnection() {
        freeaddrinfo(serverInfoList);
        close(socketFD);
    }

    std::string UDPSender::getServerHost() {
        return serverHost;
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
                LOG(FATAL) << "error in binding the socker to port: " << serverPort;
                continue;
            }

            LOG(INFO) << "receiver socker created";
            break;
        }

        if (serverAddrInfo == NULL) {
            LOG(FATAL) << "failed to create receiver socket";
        }

        freeaddrinfo(serverInfoList);
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
            std::string errorMessage = "error(" + std::to_string(numbytes) + ") occurred while receiving data";
            LOG(ERROR) << errorMessage;
            throw std::runtime_error(errorMessage);
        } else {
            buffer[numbytes] = '\0';
            std::string message(buffer);

            char host[HOST_NAME_MAX + 1];
            if (int rv = getnameinfo((struct sockaddr *) &their_addr, addr_len, host, sizeof(host), NULL, 0, 0);
                    rv != 0) {
                LOG(ERROR) << "could not get the name of the sender, error code: " << rv << ", error message: "
                           << gai_strerror(rv);
            }
            std::string receivedFrom(host);
            LOG(INFO) << "received:" << numbytes << " bytes, from: " << receivedFrom << ", message: " << message;
            return std::pair<std::string, std::string>(message, receivedFrom);
        }
    }

    void UDPReceiver::closeConnection() {
        close(sockfd);
    }
}
