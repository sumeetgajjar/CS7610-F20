#include <csignal>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <unordered_map>

#include "network_utils.h"
#include "utils.h"

DEFINE_string(hostfile, "", "path to the hostfile");
DEFINE_validator(hostfile, [](const char *, const std::string &value) {
    return !value.empty();
});

void handleSignal(int signalNum) {
    google::FlushLogFiles(google::INFO);
}

void registerSignalHandlers() {
    signal(SIGTERM, handleSignal);
    signal(SIGKILL, handleSignal);
    signal(SIGABRT, handleSignal);
    signal(SIGSEGV, handleSignal);
    signal(SIGBUS, handleSignal);
}

int main(int argc, char **argv) {
    using namespace lab2;
    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    registerSignalHandlers();

    const auto hostnames = Utils::readHostFile(FLAGS_hostfile);
    const auto currentHostname = NetworkUtils::getCurrentHostname();
    const auto peerHostnames = Utils::getPeerContainerHostnames(hostnames, currentHostname);
    const auto currentProcessIdentifier = Utils::getProcessIdentifier(hostnames, currentHostname);
    const auto recipientIdMap = [&]() {
        std::unordered_map<int, std::string> mapping;
        for (const auto &hostname : hostnames) {
            const auto recipientId = Utils::getProcessIdentifier(hostnames, hostname);
            mapping[recipientId] = hostname;
        }
        return mapping;
    }();
    return 0;
}
