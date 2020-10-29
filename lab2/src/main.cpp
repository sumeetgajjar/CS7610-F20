#include <csignal>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <thread>

#include "utils.h"
#include "membership.h"

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
    auto service = MembershipService(MEMBERSHIP_PORT, HEARTBEAT_PORT, hostnames);
    service.start();
    return 0;
}
