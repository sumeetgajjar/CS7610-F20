//
// Created by sumeet on 9/14/20.
//

#include <iostream>
#include <climits>
#include <vector>
#include <set>
#include <fstream>
#include <glog/logging.h>
#include <unistd.h>
#include "utils.h"

namespace lab0 {

    std::string Utils::getCurrentContainerHostname() {
        LOG(INFO) << "getting current host";
        char hostname[HOST_NAME_MAX];
        int errorCode = gethostname(hostname, HOST_NAME_MAX);
        if (errorCode) {
            LOG(FATAL) << "Cannot find the current host name";
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
                hostnames.push_back(hostname);
            }
        }
        hostFile.close();
        LOG(INFO) << hostnames.size() << " hosts found";
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
                peerHostnames.push_back(hostname);
            }
        }
        LOG(INFO) << peerHostnames.size() << " peers found";
        return peerHostnames;
    }

    int Utils::getProcessIdentifier(const std::vector<std::string> &allHostnames,
                                    const std::string &hostname) {
        LOG(INFO) << "getting process identifier";
        for (int i = 0; i < allHostnames.size(); i++) {
            if (allHostnames[i] == hostname) {
                int processIdentifier = i + 1;
                LOG(INFO) << "process identifier of " << hostname << " -> " << processIdentifier;
                return i + 1;
            }
        }
        LOG(FATAL) << "cannot find process identifier for: " + hostname;
    }

    std::string Utils::parseHostFileFromCmdArguments(int argc, char **argv) {
        LOG(INFO) << "parsing cmd arguments";
        if (argc != 3) {
            LOG(FATAL) << "Invalid usage, correct usage is: ./lab0 -h <hostfile>";
        }

        bool hostFilePresent = std::string(argv[1]) == "-h";
        if (!hostFilePresent) {
            LOG(FATAL) << "Invalid usage, correct usage is: ./lab0 -h <hostfile>";
        }

        LOG(INFO) << "cmd arguments parsed";
        return std::string(argv[2]);
    }
}