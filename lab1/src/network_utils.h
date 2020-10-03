//
// Created by sumeet on 9/30/20.
//

#ifndef LAB1_NETWORK_UTILS_H
#define LAB1_NETWORK_UTILS_H

#include <string>
#include <netdb.h>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <memory>

#define MAX_UDP_BUFFER_SIZE 1024
#define HEARTBEAT_PORT 7610

namespace lab1 {
    class NetworkUtils {
    public:
        static std::string getCurrentContainerHostname();

        static std::string getHostnameFromSocket(sockaddr *sockaddr);
    };

    class UDPSender {
    private:
        std::string serverHost;
        int serverPort;
        int sendFD;
        struct addrinfo *serverInfoList, *serverAddrInfo;

        void initSocket();

    public:
        UDPSender(std::string serverHost, int serverPort);

        void send(const std::string &message);

        void close();
    };

    class UDPReceiver {
    private:
        int recvFD;
        std::string portToListen;
        char buffer[MAX_UDP_BUFFER_SIZE];

        void initSocket();

    public:
        explicit UDPReceiver(int portToListen);

        /**
         * Blocks till a message is received
         *
         * @return Returns a pair of (received message, hostname of the sender)
         */
        std::pair<std::string, std::string> receive();

        void close();
    };
}

#endif //LAB1_NETWORK_UTILS_H
