#include <iostream>
#include <glog/logging.h>

#include "socket_utils.h"
#include "utils.h"


using namespace lab0;

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    LOG(INFO) << "Testing logging";
    const auto hostFilePath = Utils::parseHostFileFromCmdArguments(argc, argv);
    const auto hostnames = Utils::readHostFile(hostFilePath);
    const auto currentContainerName = Utils::getCurrentContainerName();
    const auto otherContainerNames = Utils::getOtherContainerNames(hostnames, currentContainerName);
    const auto currentProcessNumber = Utils::getCurrentProcessNumber(hostnames, currentContainerName);

    std::cout << currentContainerName << " -> Process Number -> " << currentProcessNumber << std::endl;

//    Sender sender("127.0.0.1", 1234);
//    sender.sendMessage("Hello from sender");
//    sender.closeConnection();

    return 0;
}
