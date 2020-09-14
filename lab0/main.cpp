#include <iostream>
#include "Utils.h"

using namespace lab0;

int main(int argc, char **argv) {
    const auto hostFilePath = Utils::parseHostFileFromCmdArguments(argc, argv);
    const auto hostnames = Utils::readHostFile(hostFilePath);
    const auto currentContainerName = Utils::getCurrentContainerName();
    const auto otherContainerNames = Utils::getOtherContainerNames(hostnames, currentContainerName);
    const int currentProcessNumber = Utils::getCurrentProcessNumber(hostnames, currentContainerName);

    std::cout << currentContainerName << " -> Process Number -> " << currentProcessNumber << std::endl;
    return 0;
}
