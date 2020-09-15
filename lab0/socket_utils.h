//
// Created by sumeet on 9/14/20.
//

#ifndef LAB0_SOCKET_UTILS_H
#define LAB0_SOCKET_UTILS_H

#include <string>
#include <utility>
#include <netdb.h>

namespace lab0 {
    class Sender {
    private:
        std::string serverHost, serverPort;
        int socketFD;
        struct addrinfo *serverInfoList, *serverAddrInfo;

        void initSocket();

    public:
        Sender(std::string host, int port) : serverHost(std::move(host)), serverPort(std::to_string(port)) {
            initSocket();
        };

        void sendMessage(const std::string &message);

        void closeConnection();
    };
}

#endif //LAB0_SOCKET_UTILS_H
