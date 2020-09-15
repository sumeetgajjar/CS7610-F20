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
        static std::string getCurrentContainerHostname();

        static std::vector<std::string> readHostFile(const std::string &hostFilePath);

        static std::vector<std::string> getPeerContainerHostnames(const std::vector<std::string> &allHostnames,
                                                                  const std::string &currentHostname);

        static int getProcessIdentifier(const std::vector<std::string> &allHostnames,
                                        const std::string &currentHostname);

        static std::string parseHostFileFromCmdArguments(int argc, char **argv);
    };
}


#endif //LAB0_UTILS_H
