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

    static std::vector<std::string> readHostFile() {
        std::vector<std::string> hostnames;
        std::set<std::string> uniqueHostnames;

        std::ifstream hostfile(HOST_FILE_PATH);
        std::string line;
        while (std::getline(hostfile, line)) {
            const std::string &hostname = line;
            if (uniqueHostnames.insert(hostname).second) {
                hostnames.push_back(hostname);
            }
        }
        hostfile.close();
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
};

class Receiver {

};

class Sender {

};

int main() {
    const auto hostnames = Utils::readHostFile();
    const auto currentHostname = Utils::getCurrentHostname();
    const auto otherProcessesHostnames = Utils::getHostnameOfOthers(hostnames, currentHostname);
    const int currentProcessNumber = Utils::getCurrentProcessNumber(hostnames, currentHostname);

    for (const auto hostname : hostnames) {
        std::cout << hostname.length() << " " << hostname << std::endl;
    }
    std::cout << "Process Number -> " << currentProcessNumber << std::endl;

    for (const auto hostname: otherProcessesHostnames) {
        std::cout << hostname.length() << " " << hostname << std::endl;
    }

    return 0;
}
