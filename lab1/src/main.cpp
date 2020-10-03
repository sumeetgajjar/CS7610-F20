#include <glog/logging.h>
#include <gflags/gflags.h>

#include "flags.h"
#include "network_utils.h"
#include "utils.h"


using namespace lab1;

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    FLAGS_v = 1;
    const auto hostnames = Utils::readHostFile(FLAGS_hostfilePath);
    const auto currentContainerHostname = NetworkUtils::getCurrentContainerHostname();
    const auto peerContainerHostnames = Utils::getPeerContainerHostnames(hostnames, currentContainerHostname);
    const auto currentProcessIdentifier = Utils::getProcessIdentifier(hostnames, currentContainerHostname);

    UDPTransport udpTransport = UDPTransport("beta", 1234);
    udpTransport.send("asd");
    auto pair = udpTransport.receive();
    LOG(INFO) << pair.first << " <- " << pair.second;

    return 0;
}
