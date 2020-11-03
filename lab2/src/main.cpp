#include <csignal>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <thread>

#include "utils.h"
#include "membership.h"
#include "failure_detector.h"

#define MEMBERSHIP_PORT 10000
#define HEARTBEAT_PORT 10001

DEFINE_string(hostfile, "", "path to the hostfile");
DEFINE_validator(hostfile, [](const char *, const std::string &value) {
    return !value.empty();
});
DEFINE_bool(leaderFailureDemo, false, "if enabled demo leader failure");

void handleSignal(int signalNum) {
    google::FlushLogFiles(google::INFO);
}

void registerSignalHandlers() {
    signal(SIGTERM, handleSignal);
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
    PeerInfo::initialize(NetworkUtils::getCurrentHostname(), hostnames);

    FailureDetector failureDetector(HEARTBEAT_PORT);
    MembershipService membershipService(MEMBERSHIP_PORT, [&]() { return failureDetector.getAlivePeers(); });
    failureDetector.addPeerFailureCallback([&](PeerId crashedPeerId) {
        membershipService.handlePeerFailure(crashedPeerId);
    });

    std::thread membershipServiceThread([&]() {
        VLOG(1) << "starting membership service thread";
        membershipService.start();
    });

    std::thread failureDetectorThread([&]() {
        VLOG(1) << "starting failure detector service thread";
        failureDetector.start();
    });

    membershipServiceThread.join();
    failureDetectorThread.join();
    return 0;
}
