#include <arpa/inet.h>
#include <climits>
#include <glog/logging.h>
#include <thread>

#include "network_utils.h"

namespace lab2 {

    TransportException::TransportException(const std::string &message) : std::runtime_error(message) {}

    std::string NetworkUtils::getCurrentHostname() {
        VLOG(1) << "getting current host";
        char hostname[HOST_NAME_MAX];
        int status = gethostname(hostname, HOST_NAME_MAX);
        CHECK(status == 0) << ", cannot find the current host name, errorCode: " << errno;
        LOG(INFO) << "current host name: " << hostname;
        return std::string(hostname);
    }

    std::string NetworkUtils::getHostnameFromSocket(sockaddr_storage *sockaddrStorage) {
        struct sockaddr *sockaddr = (struct sockaddr *) sockaddrStorage;
        char host[HOST_NAME_MAX + 1];
        int rv = getnameinfo(sockaddr, sizeof(*sockaddr), host, sizeof(host), nullptr, 0, 0);
        CHECK(rv == 0) << ", could not get the name of the sender, error code: " << rv
                       << ", error message: " << gai_strerror(rv);
        return std::string(host);
    }

    std::string NetworkUtils::getServiceNameFromSocket(sockaddr_storage *sockaddrStorage) {
        struct sockaddr *sockaddr = (struct sockaddr *) sockaddrStorage;
        char port[HOST_NAME_MAX + 1];
        int rv = getnameinfo(sockaddr, sizeof(*sockaddr), nullptr, 0, port, sizeof(port), 0);
        CHECK(rv == 0) << ", could not get the port of the sender, error code: " << rv
                       << ", error message: " << gai_strerror(rv);
        return std::string(port);
    }

    std::string NetworkUtils::parseHostnameFromSender(const std::string &sender) {
        // sender is of form <hostname>.<network-name>. e.g: "sumeet-g-alpha.cs7610-bridge"
        // However the hostfile just contains "sumeet-g-alpha", hence need to split the sender string
        return sender.substr(0, sender.find('.'));
    }

    Message::Message(const char *buffer_, size_t n, std::string sender) : n(n),
                                                                          sender(std::move(sender)) {
        memcpy(buffer, buffer_, n);
    }

    std::string Message::getParsedSender() const {
        return NetworkUtils::parseHostnameFromSender(sender);
    }

    UDPSender::UDPSender(std::string serverHost, int serverPort, int retryCount) : serverHost(std::move(serverHost)),
                                                                                   serverPort(serverPort) {
        VLOG(1) << "creating UDPSender for hostname: " << this->serverHost << ":" << this->serverPort;
        for (int i = 0; i <= retryCount; i++) {
            try {
                initSocket();
                break;
            } catch (std::runtime_error &e) {
                VLOG(1) << "UDPSender creation error: " << e.what();
                if (i >= retryCount) {
                    throw e;
                }
                std::this_thread::sleep_for(std::chrono::seconds{2});
            }
        }
    }

    void UDPSender::initSocket() {
        VLOG(1) << "inside initSocket() in UDPSender for hostname: " << serverHost << ":" << serverPort;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

        if (int rv = getaddrinfo(serverHost.c_str(), std::to_string(serverPort).c_str(), &hints, &serverInfoList);
                rv != 0) {
            throw std::runtime_error("cannot find host: " + serverHost + ", port: " + std::to_string(serverPort) +
                                     ", getaddrinfo failed: " + std::string(gai_strerror(rv)));
        }

        // loop through all the results and make a socket
        for (serverAddrInfo = serverInfoList; serverAddrInfo != nullptr; serverAddrInfo = serverAddrInfo->ai_next) {
            if ((sendFD = socket(serverAddrInfo->ai_family, serverAddrInfo->ai_socktype,
                                 serverAddrInfo->ai_protocol)) == -1) {
                LOG(ERROR) << "error in creating sender socket";
                continue;
            }
            VLOG(1) << "sender socket created for hostname: " << serverHost << ", port:" << serverPort;
            break;
        }

        CHECK(serverAddrInfo != nullptr) << ", failed to create sender socket for host: "
                                         << serverHost << ":" << serverPort;
    }

    void UDPSender::send(const char *buff, size_t size) {
        VLOG(1) << "inside send() of UDPSender, host: " << serverHost << ":" << serverPort
                << " ,buffer size: " << size;
        if (ssize_t numbytes = sendto(sendFD, buff, size, 0, serverAddrInfo->ai_addr,
                                      serverAddrInfo->ai_addrlen);
                numbytes == -1) {
            std::stringstream ss;
            ss << "error occurred while sending, host:" << serverHost << ":" << serverPort
               << ", buffer size: " << size << ", errno: " << errno;
            LOG(ERROR) << ss.str();
            throw TransportException(ss.str());
        } else {
            VLOG(1) << "UDP send to host: " << serverHost << ":" << serverPort << ", bytes: " << numbytes
                    << ", buffer size: " << size;
        }
    }

    void UDPSender::close() {
        VLOG(1) << "closing UDPSender for host: " << serverHost << ":" << serverPort;
        freeaddrinfo(serverInfoList);
        int rv = ::close(sendFD);
        LOG_IF(ERROR, rv != 0)
                        << ", error: " << errno << ", while closing UDPSender for host: " << serverHost
                        << ":" << serverPort;
    }

    UDPReceiver::UDPReceiver(int portToListen) : portToListen(std::to_string(portToListen)) {
        VLOG(1) << "creating UDPReceiver for port: " << this->portToListen;
        initSocket();
    }

    void UDPReceiver::initSocket() {
        VLOG(1) << "inside initSocket() in UDPReceiver on port: " << portToListen;
        struct addrinfo hints, *serverInfoList, *addrinfo;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP

        int rv = getaddrinfo(nullptr, portToListen.c_str(), &hints, &serverInfoList);
        CHECK(rv == 0) << ", getaddrinfo failed, port: " << portToListen << ", returnValue: "
                       << rv << ", error: " + std::string(gai_strerror(rv));

        // loop through all the results and bind to the first we can
        for (addrinfo = serverInfoList; addrinfo != nullptr; addrinfo = addrinfo->ai_next) {
            if ((recvFD = socket(addrinfo->ai_family, addrinfo->ai_socktype,
                                 addrinfo->ai_protocol)) == -1) {
                LOG(ERROR) << "error in creating receiver socket";
                continue;
            }

            if (bind(recvFD, addrinfo->ai_addr, addrinfo->ai_addrlen) == -1) {
                ::close(recvFD);
                LOG(ERROR) << "error in binding the socket";
                continue;
            }

            VLOG(1) << "receiver socket created";
            break;
        }

        CHECK(addrinfo != nullptr) << "failed to create receiver socket on port:" << portToListen;
        freeaddrinfo(serverInfoList);
    }

    Message UDPReceiver::receive() {
        VLOG(1) << "inside receive() of UDPReceiver, port: " << portToListen;
        struct sockaddr_storage their_addr;
        socklen_t addr_len;
        addr_len = sizeof(their_addr);
        char buffer[MAX_BUFFER_SIZE];
        VLOG(1) << "waiting for message";
        if (ssize_t numBytes = ::recvfrom(recvFD, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *) &their_addr,
                                          &addr_len);
                numBytes == -1) {
            std::string errorMessage("error(" + std::to_string(errno) + ") occurred while receiving data");
            LOG(ERROR) << errorMessage;
            throw TransportException(errorMessage);
        } else {
            std::string receivedFrom = NetworkUtils::getHostnameFromSocket(&their_addr);
            VLOG(1) << "received:" << numBytes << " bytes, on port: " << portToListen << ", from: " << receivedFrom;
            return Message(buffer, numBytes, receivedFrom);
        }
    }

    void UDPReceiver::close() {
        VLOG(1) << "closing UDPReceiver on port: " << portToListen;
        int rv = ::close(recvFD);
        LOG_IF(ERROR, rv != 0) << ", error: " << errno
                               << ", while closing UDPReceiver on port: " << portToListen;
    }

    int bindSocketToFd(const std::string &hostname, const int port, struct addrinfo hints,
                       struct addrinfo **serverInfoList, struct addrinfo **serverAddrInfo) {
        VLOG(1) << "inside bindSocketToFd hostname: " << hostname << ":" << port;

        if (int rv = ::getaddrinfo(hostname.empty() ? nullptr : hostname.c_str(),
                                   std::to_string(port).c_str(), &hints, serverInfoList);
                rv != 0) {
            throw std::runtime_error("cannot find host: " + hostname + ", port: " + std::to_string(port) +
                                     ", getaddrinfo failed: " + std::string(::gai_strerror(rv)));
        }

        int sockFd;
        // loop through all the results and make a socket
        struct addrinfo *p;
        for (p = *serverInfoList; p != nullptr; p = p->ai_next) {
            if ((sockFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                LOG(ERROR) << "error in creating sender socket";
                continue;
            }
            *serverAddrInfo = p;
            VLOG(1) << "sender socket created for hostname: " << hostname << ", port:" << port;
            break;
        }

        CHECK(*serverAddrInfo != nullptr) << ", failed to create sender socket for host: "
                                          << hostname << ":" << port;
        return sockFd;
    }

    TcpServer::TcpServer(const int listeningPort) : listeningPort(listeningPort) {
        VLOG(1) << "creating tcp server on port: " << listeningPort;
        initSocket();
    }

    void TcpServer::initSocket() {
        VLOG(1) << "inside initSocket() of TcpServer on port: " << listeningPort;
        struct addrinfo hints, *serverInfoList, *serverAddrInfo;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        sockFd = bindSocketToFd("", listeningPort, hints, &serverInfoList, &serverAddrInfo);
        int yes = 1;
        VLOG(1) << "setting socket opt";
        CHECK(::setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) != -1)
                        << ", tcp server: setsockopt error for port" << listeningPort;
        VLOG(1) << "binding port";
        CHECK(::bind(sockFd, serverAddrInfo->ai_addr, serverAddrInfo->ai_addrlen) != -1)
                        << ", tcp server: binding error for port" << listeningPort;
        CHECK(serverAddrInfo != nullptr)
                        << ", tcp server failed to bind for port" << listeningPort;
        ::freeaddrinfo(serverInfoList);
        VLOG(1) << "starting listening on port: " << listeningPort;
        CHECK(::listen(sockFd, TCP_BACKLOG_QUEUE_SIZE) != -1)
                        << ", tcp server failed to listen for port" << listeningPort;
    }

    TcpClient TcpServer::accept() const {
        VLOG(1) << "waiting for connections on port: " << listeningPort;
        struct sockaddr_storage clientAddr;
        socklen_t sin_size = sizeof(clientAddr);
        int clientFd = ::accept(sockFd, (struct sockaddr *) &clientAddr, &sin_size);
        const std::string clientHostname = NetworkUtils::getHostnameFromSocket(&clientAddr);
        const int clientPort = std::stoi(NetworkUtils::getServiceNameFromSocket(&clientAddr));
        VLOG(1) << "received incoming connection from " << clientHostname << ":" << clientPort;
        return TcpClient(clientFd, clientHostname, clientPort);
    }

    void TcpServer::close() {
        VLOG(1) << "closing tcp server on port: " << listeningPort;
        ::close(sockFd);
    }


    TcpClient::TcpClient(int fd, std::string hostname_, int port) : hostname(std::move(hostname_)),
                                                                    port(port),
                                                                    sockFd(fd) {
        VLOG(1) << "tcp client created for host:" << hostname << ":" << port;
        LOG_IF(FATAL, hostname.empty()) << "hostname cannot be empty";
    }

    TcpClient::TcpClient(std::string hostname_, int port, int retryCount)
            : hostname(std::move(hostname_)), port(port) {
        VLOG(1) << "creating tcp client for host: " << hostname << ":" << port;
        LOG_IF(FATAL, hostname.empty()) << "hostname cannot be empty";
        struct addrinfo hints, *serverInfoList, *serverAddrInfo;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        for (int i = 0; i <= retryCount; i++) {
            try {
                sockFd = bindSocketToFd(hostname, port, hints, &serverInfoList, &serverAddrInfo);
                CHECK(serverAddrInfo != nullptr)
                                << ", tcp client failed to connect to host: " << hostname << ":" << port;
                if (::connect(sockFd, serverAddrInfo->ai_addr, serverAddrInfo->ai_addrlen) == -1) {
                    ::close(sockFd);
                    throw std::runtime_error("cannot connect using sockFd");
                }
                break;
            } catch (const std::runtime_error &e) {
                LOG(ERROR) << "tcp client failed to connect to host: " << hostname << ":" << port;
                if (i >= retryCount) {
                    throw e;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds{1000});
                VLOG(1) << "tcp client retrying to connect to host: " << hostname << ":" << port;
            }
        }

        ::freeaddrinfo(serverInfoList);
        VLOG(1) << "tcp client created for host:" << hostname << ":" << port;
    }

    std::string TcpClient::getHostname() const {
        return NetworkUtils::parseHostnameFromSender(hostname);
    }

    int TcpClient::getPort() const {
        return port;
    }

    void TcpClient::send(const char *buff, size_t size) {
        VLOG(1) << "inside send() of tcp client for host: " << hostname << ":" << port;
        if (ssize_t numBytes = ::send(sockFd, buff, size, 0);
                numBytes == -1) {
            std::stringstream ss;
            ss << "error occurred while sending, host:" << hostname << ":" << port
               << ", buffer size: " << size << ", errno: " << errno;
            LOG(ERROR) << ss.str();
            throw TransportException(ss.str());
        } else {
            VLOG(1) << "tcp client send to host: " << hostname << ":" << port << ", bytes: " << numBytes
                    << ", buffer size: " << size;
        }
    }

    Message TcpClient::receive() {
        VLOG(1) << "inside receive() of tcp client for host: " << hostname << ":" << port;
        char buffer[MAX_BUFFER_SIZE];
        if (size_t numBytes = ::recv(sockFd, buffer, MAX_BUFFER_SIZE, 0);
                numBytes == -1) {
            std::string errorMessage("error(" + std::to_string(errno) + ") occurred while receiving data from host: " +
                                     hostname + ":" + std::to_string(port));
            LOG(ERROR) << errorMessage;
            throw TransportException(errorMessage);
        } else if (numBytes == 0) {
            throw TransportException("host: " + hostname + " crashed");
        } else {
            VLOG(1) << "received:" << numBytes << " bytes, from host: " << hostname << ":" << port;
            return Message(buffer, numBytes, hostname);
        }
    }

    void TcpClient::close() {
        VLOG(1) << "closing tcp client for host: " << hostname << ":" << port;
        ::close(sockFd);
    }
}
