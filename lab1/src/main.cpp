#include <glog/logging.h>
#include <gflags/gflags.h>
#include <csignal>

#include "network_utils.h"
#include "probe_utils.h"
#include "utils.h"
#include "multicast.h"


using namespace lab1;

DEFINE_string(hostfile, "", "path to the hostfile");
DEFINE_validator(hostfile, [](const char *, const std::string &value) {
    return !value.empty();
});

DEFINE_uint32(msgCount, 0, "number of messages to multicast");
DEFINE_double(dropRate, 0, "ratio of messages to drop");
DEFINE_uint64(delay, 0, "amount of network artificial delay");
DEFINE_uint32(initiateSnapshotCount, 0, "number of messages after which the process starts the snapshot");

void handleSignal(int signalNum) {
    google::FlushLogFiles(google::INFO);
}

void registerSignalHandlers() {
    signal(SIGTERM, handleSignal);
    signal(SIGKILL, handleSignal);
    signal(SIGABRT, handleSignal);
}

int main(int argc, char **argv) {
    try {
        google::InitGoogleLogging(argv[0]);
        gflags::ParseCommandLineFlags(&argc, &argv, true);
        registerSignalHandlers();
        const auto hostnames = Utils::readHostFile(FLAGS_hostfile);
        const auto currentContainerHostname = NetworkUtils::getCurrentContainerHostname();
        const auto peerHostnames = Utils::getPeerContainerHostnames(hostnames, currentContainerHostname);
        const auto currentProcessIdentifier = Utils::getProcessIdentifier(hostnames, currentContainerHostname);

        ProbingUtils::waitForPeersToStart(peerHostnames);

        auto multicastService = MulticastService(currentProcessIdentifier, hostnames, [](DataMessage dataMessage) {
            VLOG(1) << "got multicast message: " << dataMessage;
        }, FLAGS_dropRate, FLAGS_delay);

        std::thread multicastServiceThread([&]() {
            multicastService.start();
        });

        for (int i = 0; i < FLAGS_msgCount; ++i) {
            multicastService.multicast(i + 11);
        }

        multicastServiceThread.join();
    } catch (const std::exception &e) {
        LOG(ERROR) << "exception occurred: " << e.what();
        google::FlushLogFiles(google::INFO);
        throw e;
    };
    return 0;
}