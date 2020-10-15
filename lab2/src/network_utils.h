//
// Created by sumeet on 9/30/20.
//

#ifndef LAB1_NETWORK_UTILS_H
#define LAB1_NETWORK_UTILS_H

#include <string>
#include <netdb.h>

#define MAX_UDP_BUFFER_SIZE 1024
#define MAX_TCP_BUFFER_SIZE 1024
#define TCP_BACKLOG_QUEUE_SIZE 20

namespace lab2 {
    class NetworkUtils {
    public:
        static std::string getCurrentHostname();

        static std::string getHostnameFromSocket(sockaddr_storage *sockaddrStorage);

        static std::string getServiceNameFromSocket(sockaddr_storage *sockaddrStorage);

        static std::string parseHostnameFromSender(const std::string &sender);
    };

    class Message {
    public:
        const char *buffer;
        const size_t n;
        const std::string sender;
    public:
        Message(const char *buffer, size_t n, std::string sender);

        std::string getParsedSender() const;
    };

    class UDPSender {
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

    class UDPReceiver {
        int recvFD;
        std::string portToListen;

        void initSocket();

    public:
        explicit UDPReceiver(int portToListen);

        std::pair<int, std::string> receive(char *buffer, size_t n);

        void close();
    };

    class TcpClient {

        const std::string hostname;
        const int port;
        int sockFd;

    public:
        TcpClient(std::string hostname_, int port, int fd);

        TcpClient(std::string hostname_, int port);

        const std::string &getHostname() const;

        int getPort() const;

        void send(const char *buff, size_t size);

        Message receive();

        void close();
    };

    class TcpServer {

        const int listeningPort;
        int sockFd;

        void initSocket();

    public:
        TcpServer(int listeningPort);

        TcpClient accept() const;

        void close();
    };
}

#endif //LAB1_NETWORK_UTILS_H
