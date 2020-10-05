#include <arpa/inet.h>
#include <climits>
#include <glog/logging.h>
#include <utility>

#include "network_utils.h"

namespace lab1 {
    std::string NetworkUtils::getCurrentContainerHostname() {
        VLOG(1) << "getting current host";
        char hostname[HOST_NAME_MAX];
        int status = gethostname(hostname, HOST_NAME_MAX);
        CHECK(status == 0) << ", cannot find the current host name, errorCode: " << errno;
        LOG(INFO) << "current host name: " << hostname;
        return std::string(hostname);
    }

    std::string NetworkUtils::getHostnameFromSocket(sockaddr *sockaddr) {
        char host[HOST_NAME_MAX + 1];
        int rv = getnameinfo(sockaddr, sizeof(*sockaddr), host, sizeof(host), nullptr, 0, 0);
        CHECK(rv == 0) << ", could not get the name of the sender, error code: " << rv
                       << ", error message: " << gai_strerror(rv);
        return std::string(host);
    }

    UDPSender::UDPSender(std::string serverHost, int serverPort) : serverHost(std::move(serverHost)),
                                                                   serverPort(serverPort) {
        VLOG(1) << "creating UDPSender for hostname: " << this->serverHost << ":" << this->serverPort;
        initSocket();
    }

    void UDPSender::initSocket() {
        VLOG(1) << "inside initSocket() in UDPSender for hostname: " << serverHost << ":" << serverPort;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

        int rv = getaddrinfo(serverHost.c_str(), std::to_string(serverPort).c_str(), &hints, &serverInfoList);
        CHECK(rv == 0) << ", getaddrinfo failed, host: " << serverHost << ":" << serverPort <<
                       ", returnValue: " << rv << ", error: " + std::string(gai_strerror(rv));

        // loop through all the results and make a socket
        for (serverAddrInfo = serverInfoList; serverAddrInfo != nullptr; serverAddrInfo = serverAddrInfo->ai_next) {
            if ((sendFD = socket(serverAddrInfo->ai_family, serverAddrInfo->ai_socktype,
                                 serverAddrInfo->ai_protocol)) == -1) {
                LOG(ERROR) << "error in creating sender socket";
                continue;
            }
            LOG(INFO) << "sender socket created for hostname: " << serverHost << ", port:" << serverPort;
            break;
        }

        CHECK(serverAddrInfo != nullptr) << ", failed to create sender socket for host: "
                                         << serverHost << ":" << serverPort;
    }

    void UDPSender::send(const std::string &message) {
        VLOG(1) << "inside send() of UDPSender, host: " << serverHost << ":" << serverPort
                << " ,message: " << message;
        if (int numbytes = sendto(sendFD, message.c_str(), message.size(), 0, serverAddrInfo->ai_addr,
                                  serverAddrInfo->ai_addrlen);
                numbytes == -1) {
            LOG(ERROR) << "error occurred while sending, host:" << serverHost << ":" << serverPort
                       << ", message: " << message;
        } else {
            LOG(INFO) << "UDP send to host: " << serverHost << ":" << serverPort << ", bytes: " << numbytes
                      << ", message: " << message;
        }
    }

    void UDPSender::send(const char *buff, size_t size) {
        VLOG(1) << "inside send() of UDPSender, host: " << serverHost << ":" << serverPort
                << " ,buffer size: " << size;
        if (ssize_t numbytes = sendto(sendFD, buff, size, 0, serverAddrInfo->ai_addr,
                                      serverAddrInfo->ai_addrlen);
                numbytes == -1) {
            LOG(ERROR) << "error occurred while sending, host:" << serverHost << ":" << serverPort
                       << ", buffer size: " << size;
        } else {
            LOG(INFO) << "UDP send to host: " << serverHost << ":" << serverPort << ", bytes: " << numbytes
                      << ", buffer size: " << size;
        }
    }

    void UDPSender::close() {
        LOG(INFO) << "closing UDPSender for host: " << serverHost << ":" << serverPort;
        freeaddrinfo(serverInfoList);
        int rv = ::close(sendFD);
        LOG_IF(ERROR, rv != 0)
                        << ", error: " << errno << ", while closing UDPSender for host: " << serverHost
                        << ":" << serverPort;
    }

    Message::Message(const char *buffer, size_t n, std::string sender) : buffer(buffer),
                                                                         n(n),
                                                                         sender(std::move(sender)) {}

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

            LOG(INFO) << "receiver socket created";
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

        LOG(INFO) << "waiting for message";
        char buffer[MAX_UDP_BUFFER_SIZE];
        memset(&buffer, 0, MAX_UDP_BUFFER_SIZE);
        if (ssize_t numbytes = recvfrom(recvFD, buffer, MAX_UDP_BUFFER_SIZE - 1, 0, (struct sockaddr *) &their_addr,
                                        &addr_len);
                numbytes == -1) {
            std::string errorMessage("error(" + std::to_string(numbytes) + ") occurred while receiving data");
            LOG(ERROR) << errorMessage;
            throw std::runtime_error(errorMessage);
        } else {
            buffer[numbytes] = '\0';
            std::string receivedFrom = NetworkUtils::getHostnameFromSocket((struct sockaddr *) &their_addr);
            LOG(INFO) << "received:" << numbytes << " bytes, on port: " << portToListen << ", from: " << receivedFrom;
            return Message(buffer, numbytes, receivedFrom);
        }
    }

    void UDPReceiver::close() {
        LOG(INFO) << "closing UDPReceiver on port: " << portToListen;
        int rv = ::close(recvFD);
        LOG_IF(ERROR, rv != 0) << ", error: " << errno
                               << ", while closing UDPReceiver on port: " << portToListen;
    }
}
