//
// Created by sumeet on 9/30/20.
//

#ifndef LAB1_NETWORK_UTILS_H
#define LAB1_NETWORK_UTILS_H

#include <limits>
#include <string>
#include <netdb.h>

#define MAX_BUFFER_SIZE 1024
#define TCP_BACKLOG_QUEUE_SIZE 20

namespace lab2 {

    class NetworkUtils {
    public:
        static std::string getCurrentHostname();

        static std::string getHostnameFromSocket(sockaddr_storage *sockaddrStorage);

        static std::string getServiceNameFromSocket(sockaddr_storage *sockaddrStorage);

        static std::string parseHostnameFromSender(const std::string &sender);
    };

    class TransportException : public std::runtime_error {
    public:
        explicit TransportException(const std::string &message);
    };

    class Message {
    public:
        const size_t n;
        const std::string sender;
        char buffer[MAX_BUFFER_SIZE];
    public:
        Message(const char *buffer_, size_t n, std::string sender);

        std::string getParsedSender() const;
    };

    class UDPSender {
        std::string serverHost;
        int serverPort;
        int sendFD;
        struct addrinfo *serverInfoList, *serverAddrInfo;

        void initSocket();

    public:
        UDPSender(std::string serverHost, int serverPort, int retryCount = std::numeric_limits<int>::max());

        void send(const char *buff, size_t size);

        void close();
    };

    class UDPReceiver {
        int recvFD;
        std::string portToListen;

        void initSocket();

    public:
        explicit UDPReceiver(int portToListen);

        Message receive();

        void close();
    };

    class TcpClient {

        const std::string hostname;
        const int port;
        int sockFd;

    public:
        TcpClient(int fd, std::string hostname_, int port);

        TcpClient(std::string hostname_, int port, int retryCount = std::numeric_limits<int>::max());

        std::string getHostname() const;

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
