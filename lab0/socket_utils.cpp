//
// Created by sumeet on 9/14/20.
//

#include "socket_utils.h"
#include <cstring>
#include <unistd.h>
#include <glog/logging.h>

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
                LOG(ERROR) << "creating socket";
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
}