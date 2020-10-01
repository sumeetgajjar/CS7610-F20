#include "flags.h"
#include "utils.h"

#include <glog/logging.h>
#include <gflags/gflags.h>


using namespace lab1;

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    const auto hostnames = Utils::readHostFile(FLAGS_hostfilePath);
    const auto currentContainerHostname = Utils::getCurrentContainerHostname();
    const auto peerContainerHostnames = Utils::getPeerContainerHostnames(hostnames, currentContainerHostname);
    const auto currentProcessIdentifier = Utils::getProcessIdentifier(hostnames, currentContainerHostname);

    return 0;
}
