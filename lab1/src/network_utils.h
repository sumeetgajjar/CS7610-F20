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

        void send(const char *buff, size_t size);

        void close();
    };

    class Message {
    public:
        const char *buffer;
        const size_t n;
        const std::string sender;

    public:
        Message(const char *buffer, size_t n, std::string sender);
    };

    class UDPReceiver {
    private:
        int recvFD;
        std::string portToListen;

        void initSocket();

    public:
        explicit UDPReceiver(int portToListen);

        Message receive();

        void close();
    };
}

#endif //LAB1_NETWORK_UTILS_H
