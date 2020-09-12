#include <iostream>
#include <climits>
#include <vector>
#include <set>
#include <fstream>
#include <unistd.h>
#include "config.h"

class Utils {
public:
    static std::string getCurrentHostname() {
        char hostname[HOST_NAME_MAX];
        int result = gethostname(hostname, HOST_NAME_MAX);
        if (result) {
            throw std::runtime_error("Cannot find the hostname");
        }
        return std::string(hostname);
    }

    static std::vector<std::string> readHostFile(std::string hostFilePath) {
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

    static std::vector<std::string> getHostnameOfOthers(std::vector<std::string> hostnames,
                                                        std::string currentHostname) {
        std::vector<std::string> otherHostnames;
        for (const auto hostname: hostnames) {
            if (hostname != currentHostname) {
                otherHostnames.push_back(hostname);
            }
        }
        return otherHostnames;
    }

    static int getCurrentProcessNumber(std::vector<std::string> hostnames,
                                       std::string currentHostname) {
        for (int i = 0; i < hostnames.size(); i++) {
            if (hostnames[i] == currentHostname) {
                return i + 1;
            }
        }
        throw std::runtime_error("cannot find current hostname: " + currentHostname + " in the hostfile");
    }

    static std::string parseHostFileFromCmdArguments(int argc, char **argv) {
        if (argc != 3) {
            throw std::invalid_argument("Invalid usage, correct usage is: ./lab0 -h <hostfile>");
        }

        bool hostFilePresent = std::string(argv[1]) == "-h";
        if (!hostFilePresent) {
            throw std::invalid_argument("Invalid usage, correct usage is: ./lab0 -h <hostfile>");
        }

        return std::string(argv[2]);
    }
};

class Receiver {

};

class Sender {

};

int main(int argc, char **argv) {
    const auto hostFilePath = Utils::parseHostFileFromCmdArguments(argc, argv);
    const auto hostnames = Utils::readHostFile(hostFilePath);
    const auto currentHostname = Utils::getCurrentHostname();
    const auto otherProcessesHostnames = Utils::getHostnameOfOthers(hostnames, currentHostname);
    const int currentProcessNumber = Utils::getCurrentProcessNumber(hostnames, currentHostname);

    std::cout << currentHostname << " -> Process Number -> " << currentProcessNumber << std::endl;
    return 0;
}
