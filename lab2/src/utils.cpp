//
// Created by sumeet on 9/30/20.
//

#include "utils.h"

#include <iostream>
#include <vector>
#include <set>
#include <fstream>
#include <glog/logging.h>
#include <random>

namespace lab2 {

    static std::default_random_engine randomEngine(11);


    std::vector<std::string> Utils::readHostFile(const std::string &hostFilePath) {
        VLOG(1) << "reading host file, path: " << hostFilePath;
        std::vector<std::string> hostnames;
        std::set<std::string> uniqueHostnames;

        std::ifstream hostFile(hostFilePath);
        std::string line;
        while (std::getline(hostFile, line)) {
            const std::string &hostname = line;
            if (uniqueHostnames.insert(hostname).second) {
                hostnames.emplace_back(hostname);
            }
        }
        hostFile.close();
        LOG(INFO) << hostnames.size() << " hosts found in hostfile";
        CHECK(hostnames.size() >= 5) << ", hostfile should contain more than 5 hosts";
        CHECK(hostnames.size() <= 10) << ", hostfile cannot contain more than 10 hosts";
        return hostnames;
    }

    std::vector<std::string> Utils::getPeerContainerHostnames(const std::vector<std::string> &allHostnames,
                                                              const std::string &currentHostname) {
        VLOG(1) << "getting peer container host names";
        std::vector<std::string> peerHostnames;
        for (const auto &hostname: allHostnames) {
            if (hostname != currentHostname) {
                peerHostnames.emplace_back(hostname);
            }
        }
        LOG(INFO) << peerHostnames.size() << " peers found";
        return peerHostnames;
    }

    uint32_t Utils::getProcessIdentifier(const std::string &hostname, const std::vector<std::string> &allHostnames) {
        VLOG(1) << "getting process identifier";
        for (unsigned int i = 0; i < allHostnames.size(); i++) {
            if (allHostnames.at(i) == hostname) {
                unsigned int processIdentifier = i + 1;
                LOG(INFO) << "process identifier of " << hostname << " -> " << processIdentifier;
                return processIdentifier;
            }
        }
        LOG(FATAL) << "cannot find process identifier for: " + hostname;
    }

    double Utils::getRandomNumber(double min, double max) {
        std::uniform_real_distribution<double> distribution(min, max);
        return distribution(randomEngine);
    }

    PeerId PeerInfo::myPeerId(-1);
    std::vector<std::string> PeerInfo::allPeerHostnames;
    std::unordered_map<std::string, PeerId> PeerInfo::hostnameToPeerIdMap;
    std::unordered_map<PeerId, std::string> PeerInfo::peerIdToHostnameMap;


    void PeerInfo::initialize(const std::string &myHostname, std::vector<std::string> allPeerHostnames_) {
        allPeerHostnames = std::move(allPeerHostnames_);
        for (const auto &hostname : allPeerHostnames) {
            auto peerId = Utils::getProcessIdentifier(hostname, allPeerHostnames);
            hostnameToPeerIdMap[hostname] = peerId;
            peerIdToHostnameMap[peerId] = hostname;
        }

        myPeerId = hostnameToPeerIdMap.at(myHostname);
        LOG(INFO) << "myPeerId: " << myPeerId;
    }

    PeerId PeerInfo::getMyPeerId() {
        return myPeerId;
    }

    std::string PeerInfo::getHostname(const PeerId peerId) {
        return peerIdToHostnameMap.at(peerId);
    }

    PeerId PeerInfo::getPeerId(const std::string &hostname) {
        return hostnameToPeerIdMap.at(hostname);
    }

    const std::vector<std::string> &PeerInfo::getAllPeerHostnames() {
        return allPeerHostnames;
    }
}