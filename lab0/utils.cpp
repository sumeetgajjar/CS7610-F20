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

    std::string Utils::getCurrentContainerName() {
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

    std::vector<std::string> Utils::getOtherContainerNames(const std::vector<std::string> &hostnames,
                                                           const std::string &currentHostname) {
        std::vector<std::string> otherHostnames;
        for (const auto &hostname: hostnames) {
            if (hostname != currentHostname) {
                otherHostnames.push_back(hostname);
            }
        }
        return otherHostnames;
    }

    int Utils::getCurrentProcessNumber(const std::vector<std::string> &hostnames,
                                       const std::string &currentHostname) {
        for (int i = 0; i < hostnames.size(); i++) {
            if (hostnames[i] == currentHostname) {
                return i + 1;
            }
        }
        throw std::runtime_error("cannot find current hostname: " + currentHostname + " in the hostfile");
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