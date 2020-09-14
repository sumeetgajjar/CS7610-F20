//
// Created by sumeet on 9/14/20.
//

#ifndef LAB0_UTILS_H
#define LAB0_UTILS_H


#include <string>
#include <vector>

namespace lab0 {
    class Utils {
    public:
        static std::string getCurrentContainerName();

        static std::vector<std::string> readHostFile(const std::string &hostFilePath);

        static std::vector<std::string> getOtherContainerNames(const std::vector<std::string> &hostnames,
                                                               const std::string &currentHostname);

        static int getCurrentProcessNumber(const std::vector<std::string> &hostnames,
                                           const std::string &currentHostname);

        static std::string parseHostFileFromCmdArguments(int argc, char **argv);
    };
}


#endif //LAB0_UTILS_H
