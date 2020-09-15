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

    class Receiver {
    private:
        static const int MAX_BUFFER_SIZE = 100;
        int sockfd;
        struct addrinfo *serverAddrInfo;
        std::string serverPort;
        char buffer[MAX_BUFFER_SIZE];

        void initSocket();

    public:
        Receiver(int port) : serverPort(std::to_string(port)) {
            initSocket();
        }

        /**
         * Blocks till a message is received
         * @return Returns a pair of (received message, hostname of the sender)
         */
        std::pair<std::string, std::string> receiveMessage();

        void closeConnection();
    };
}

#endif //LAB0_SOCKET_UTILS_H
