//
// Created by sumeet on 9/30/20.
//

#ifndef LAB1_UTILS_H
#define LAB1_UTILS_H


#include <string>
#include <vector>

namespace lab1 {
    class Utils {
    public:

        static std::vector<std::string> readHostFile(const std::string &hostFilePath);

        static std::vector<std::string> getPeerContainerHostnames(const std::vector<std::string> &allHostnames,
                                                                  const std::string &currentHostname);

        static uint32_t getProcessIdentifier(const std::vector<std::string> &allHostnames,
                                             const std::string &hostname);

        static double getRandomNumber(double min = 0, double max = 1);
    };
}


#endif //LAB1_UTILS_H
