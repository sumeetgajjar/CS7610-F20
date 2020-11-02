//
// Created by sumeet on 9/30/20.
//

#ifndef LAB2_UTILS_H
#define LAB2_UTILS_H


#include <string>
#include <vector>
#include <unordered_map>
#include "message.h"

namespace lab2 {

    class Utils {
    public:

        static std::vector<std::string> readHostFile(const std::string &hostFilePath);

        static std::vector<std::string> getPeerContainerHostnames(const std::vector<std::string> &allHostnames,
                                                                  const std::string &currentHostname);

        static uint32_t getProcessIdentifier(const std::string &hostname, const std::vector<std::string> &allHostnames);

        static double getRandomNumber(double min = 0, double max = 1);
    };

    class PeerInfo {
        static PeerId myPeerId;
        static std::vector<std::string> allPeerHostnames;
        static std::unordered_map<std::string, PeerId> hostnameToPeerIdMap;
        static std::unordered_map<PeerId, std::string> peerIdToHostnameMap;
    public:
        static void initialize(const std::string &myHostname, std::vector<std::string> allPeerHostnames_);

        static PeerId getMyPeerId();

        static std::string getHostname(PeerId peerId);

        static PeerId getPeerId(const std::string &hostname);

        static const std::vector<std::string> &getAllPeerHostnames();
    };
}


#endif //LAB2_UTILS_H
