#include <glog/logging.h>
#include <gflags/gflags.h>
#include <csignal>

#include "multicast.h"
#include "network_utils.h"
#include "snapshot.h"
#include "utils.h"


using namespace lab1;

DEFINE_string(hostfile, "", "path to the hostfile");
DEFINE_validator(hostfile, [](const char *, const std::string &value) {
    return !value.empty();
});
DEFINE_string(senders, "", "comma separated list of hostnames of the senders");
DEFINE_uint32(msgCount, 0, "number of messages to multicast");
DEFINE_double(dropRate, 0, "ratio of messages to drop");
DEFINE_uint64(delay, 0, "amount of network artificial delay in millis");
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

        const auto sendMulticastMsg = [&]() {
            std::stringstream ss(FLAGS_senders);
            std::string sender;
            while (getline(ss, sender, ',')) {
                if (sender == currentHostname) {
                    LOG(INFO) << currentHostname << " is part of senders group";
                    return true;
                }
            }
            return false;
        }();

        auto snapshotService = SnapshotService(currentProcessIdentifier, peerHostnames);

        auto multicastService = MulticastService(currentProcessIdentifier,
                                                 hostnames,
                                                 recipientIdMap,
                                                 [](const DataMessage &dataMessage) {
                                                     VLOG(1) << "got multicast message: " << dataMessage;
                                                 },
                                                 [&](const Message &message) {
                                                     snapshotService.recordIncomingMessages(message);
                                                 },
                                                 FLAGS_dropRate,
                                                 FLAGS_delay);

        snapshotService.setLocalStateGetter([&]() { return multicastService.getCurrentState(); });

        std::thread multicastServiceThread([&]() { multicastService.start(); });
        std::thread snapshotServiceThread([&]() { snapshotService.start(); });

        if (sendMulticastMsg) {
            bool snapshotTaken = false;
            for (int i = 1; i <= FLAGS_msgCount; ++i) {
                multicastService.multicast(i + 10);
                if (!snapshotTaken && FLAGS_initiateSnapshotCount && i > FLAGS_initiateSnapshotCount) {
                    snapshotService.takeSnapshot();
                    snapshotTaken = true;
                }
            }
        }

        multicastServiceThread.join();
        snapshotServiceThread.join();
    } catch (const std::exception &e) {
        LOG(ERROR) << "exception occurred: " << e.what();
        google::FlushLogFiles(google::INFO);
        throw e;
    };
    return 0;
}