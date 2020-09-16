//
// Created by sumeet on 9/14/20.
//

#include <iostream>
#include <climits>
#include <vector>
#include <set>
#include <fstream>
#include <unistd.h>
#include "utils.h"

namespace lab0 {

    std::string Utils::getCurrentContainerHostname() {
        // TODO: change the implementation to use the container name, wait for rahul to confirm
        char hostname[HOST_NAME_MAX];
        int errorCode = gethostname(hostname, HOST_NAME_MAX);
        if (errorCode) {
            throw std::runtime_error("Cannot find the container name");
        }
        return std::string(hostname);
    }

    std::vector<std::string> Utils::readHostFile(const std::string &hostFilePath) {
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
        return hostnames;
    }

    std::vector<std::string> Utils::getPeerContainerHostnames(const std::vector<std::string> &allHostnames,
                                                              const std::string &currentHostname) {
        std::vector<std::string> peerHostnames;
        for (const auto &hostname: allHostnames) {
            if (hostname != currentHostname) {
                peerHostnames.push_back(hostname);
            }
        }
        return peerHostnames;
    }

    int Utils::getProcessIdentifier(const std::vector<std::string> &allHostnames,
                                    const std::string &hostname) {
        for (int i = 0; i < allHostnames.size(); i++) {
            if (allHostnames[i] == hostname) {
                return i + 1;
            }
        }
        throw std::runtime_error("cannot find current hostname: " + hostname + " in the hostfile");
    }

    std::string Utils::parseHostFileFromCmdArguments(int argc, char **argv) {
        if (argc != 3) {
            throw std::invalid_argument("Invalid usage, correct usage is: ./lab0 -h <hostfile>");
        }

        bool hostFilePresent = std::string(argv[1]) == "-h";
        if (!hostFilePresent) {
            throw std::invalid_argument("Invalid usage, correct usage is: ./lab0 -h <hostfile>");
        }

        return std::string(argv[2]);
    }
}