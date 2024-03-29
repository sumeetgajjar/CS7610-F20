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

namespace lab1 {

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
        CHECK(hostnames.size() >= 3) << ", hostfile should contain more than 3 hosts";
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

    uint32_t Utils::getProcessIdentifier(const std::vector<std::string> &allHostnames,
                                         const std::string &hostname) {
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
}