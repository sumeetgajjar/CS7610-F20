//
// Created by sumeet on 9/30/20.
//

#include "utils.h"

#include <iostream>
#include <climits>
#include <vector>
#include <set>
#include <fstream>
#include <glog/logging.h>
#include <unistd.h>

namespace lab1 {

    std::string Utils::getCurrentContainerHostname() {
        LOG(INFO) << "getting current host";
        char hostname[HOST_NAME_MAX];
        int errorCode = gethostname(hostname, HOST_NAME_MAX);
        if (errorCode) {
            LOG(FATAL) << "cannot find the current host name";
        }
        LOG(INFO) << "current host name: " << hostname;
        return std::string(hostname);
    }

    std::vector<std::string> Utils::readHostFile(const std::string &hostFilePath) {
        LOG(INFO) << "reading host file, path: " << hostFilePath;
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
        if (hostnames.size() < 2) {
            LOG(FATAL) << "hostfile should contain more than 1 hosts";
        }
        return hostnames;
    }

    std::vector<std::string> Utils::getPeerContainerHostnames(const std::vector<std::string> &allHostnames,
                                                              const std::string &currentHostname) {
        LOG(INFO) << "getting peer container host names";
        std::vector<std::string> peerHostnames;
        for (const auto &hostname: allHostnames) {
            if (hostname != currentHostname) {
                peerHostnames.emplace_back(hostname);
            }
        }
        LOG(INFO) << peerHostnames.size() << " peers found";
        return peerHostnames;
    }

    unsigned int Utils::getProcessIdentifier(const std::vector<std::string> &allHostnames,
                                             const std::string &hostname) {
        LOG(INFO) << "getting process identifier";
        for (unsigned int i = 0; i < allHostnames.size(); i++) {
            if (allHostnames[i] == hostname) {
                unsigned int processIdentifier = i + 1;
                LOG(INFO) << "process identifier of " << hostname << " -> " << processIdentifier;
                return processIdentifier;
            }
        }
        LOG(FATAL) << "cannot find process identifier for: " + hostname;
    }
}