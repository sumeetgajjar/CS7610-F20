//
// Created by sumeet on 9/14/20.
//

#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <glog/logging.h>

#include "socket_utils.h"

namespace lab0 {

    void Sender::initSocket() {
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

    void Sender::sendMessage(const std::string &message) {
        LOG(INFO) << "sending message: " << message;
        if (int numbytes = sendto(socketFD, message.c_str(), message.size(), 0, serverAddrInfo->ai_addr,
                                  serverAddrInfo->ai_addrlen);
                numbytes < 0) {
            LOG(ERROR) << "error(" << numbytes << ") occurred while sending message: " << message;
        } else {
            LOG(INFO) << "sent to " << serverHost << ", bytes: " << numbytes << ", message: " << message;
        }
    }

    void Sender::closeConnection() {
        freeaddrinfo(serverInfoList);
        close(socketFD);
    }

    void Receiver::initSocket() {
        struct addrinfo hints, *serverInfoList;
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

    void *get_in_addr(struct sockaddr *sa) {
        if (sa->sa_family == AF_INET) {
            return &(((struct sockaddr_in *) sa)->sin_addr);
        }

        return &(((struct sockaddr_in6 *) sa)->sin6_addr);
    }

    std::pair<std::string, std::string> Receiver::receiveMessage() {
        struct sockaddr_storage their_addr;
        socklen_t addr_len;
        addr_len = sizeof(their_addr);

        LOG(INFO) << "waiting for message";
        memset(&buffer, 0, MAX_BUFFER_SIZE);
        if (int numbytes = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE - 1, 0, (struct sockaddr *) &their_addr,
                                    &addr_len);
                numbytes < 0) {
            std::string errorMessage = "error(" + std::to_string(numbytes) + ") occurred while receiving data";
            LOG(ERROR) << errorMessage;
            throw std::runtime_error(errorMessage);
        } else {
            char s[INET6_ADDRSTRLEN];
            auto receivedFrom = inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr), s,
                                          sizeof(s));
            buffer[numbytes] = '\0';
            std::string message(buffer);
            LOG(INFO) << "received:" << numbytes << " bytes, from: " << receivedFrom << ", message: " << message;
            return std::pair<std::string, std::string>(message, receivedFrom);
        }
    }

    void Receiver::closeConnection() {
        close(sockfd);
    }
}
