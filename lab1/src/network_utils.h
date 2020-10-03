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
#include "constants.h"

namespace lab1 {
    class NetworkUtils {
    public:
        static std::string getCurrentContainerHostname();

        static std::string getHostnameFromSocket(sockaddr *sockaddr);
    };

    class UDPTransport {
        // common info
        const std::string hostname;
        const int port;
        const int recvPort;

        // for sending
        int sendFD = -1;
        struct addrinfo *serverInfoList = nullptr, *serverAddrInfo = nullptr;

        // for receiving
        int recvFD = -1;
        char recvBuffer[MAX_UDP_BUFFER_SIZE];

        void initReceiver();

        void initSender();

    public:
        UDPTransport(std::string hostname, int port);

        void send(const std::string &message);

        /**
         * Blocks till a message is received
         *
         * @return Returns a pair of (received message, hostname of the sender)
         */
        std::pair<std::string, std::string> receive();

        void close();
    };

    class HeartbeatSender {
    private:
        std::mutex aliveMessageMutex;
        std::unordered_set<std::string> aliveMessageReceivers;
        std::mutex transportMutex;
        std::unordered_map<std::string, std::shared_ptr<UDPTransport>> udpTransport;
        std::atomic_bool stopAliveMessageLoop;
        const int messageDelay = 4;

        std::shared_ptr<UDPTransport> getUDPTransport(const std::string &hostname);

    public:

        static const std::string aliveMessage;
        static const std::string ackMessage;

        void startSendingAliveMessages();

        void addToAliveMessageReceiverList(const std::string &hostname);

        void removeFromAliveMessageReceiverList(const std::string &hostname);

        int getAliveMessageReceiverListSIze();

        void sendingAckMessages(const std::string &hostname);

    };

    class HeartbeatReceiver {
    private:
        UDPTransport udpTransport;
        std::unordered_set<std::string> validSenders;
    public:
        HeartbeatReceiver(UDPTransport &udpReceiver, std::unordered_set<std::string> validSenders);

        void startListeningForMessages();
    };
}

#endif //LAB1_NETWORK_UTILS_H
