//
// Created by sumeet on 9/14/20.
//

#ifndef LAB0_SOCKET_UTILS_H
#define LAB0_SOCKET_UTILS_H

#include <string>
#include <utility>
#include <netdb.h>

namespace lab0 {

    class SocketUtils {
    public:
        static std::string getHostnameFromSocket(sockaddr *sockaddr);
    };

    class UDPSender {
    private:
        std::string serverHost, serverPort;
        int socketFD;
        struct addrinfo *serverInfoList, *serverAddrInfo;

        void initSocket();

    public:
        UDPSender(std::string host, int port) : serverHost(std::move(host)), serverPort(std::to_string(port)) {
            initSocket();
        };

        void sendMessage(const std::string &message);

        void closeConnection();
    };

    class UDPReceiver {
    private:
        static const int MAX_BUFFER_SIZE = 100;
        int sockfd;
        std::string serverPort;
        char buffer[MAX_BUFFER_SIZE];

        void initSocket();

    public:
        UDPReceiver(int port) : serverPort(std::to_string(port)) {
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
