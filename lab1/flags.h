//
// Created by sumeet on 9/30/20.
//

#ifndef LAB1_FLAGS_H
#define LAB1_FLAGS_H

#include <gflags/gflags.h>
#include <glog/logging.h>

DEFINE_string(hostfilePath, "", "path to the hostfile");
DEFINE_validator(hostfilePath, [](const char *flagname, const std::string &value) {
    return !value.empty();
});

DEFINE_uint32(multicastMessageCount, 0, "number of messages to multicast");
DEFINE_double(dropRate, 0, "ratio of messages to drop");
DEFINE_uint64(delay, 0, "amount of network artificial delay");
DEFINE_uint32(intiateSnapshotCount, -1, "number of messages after which the process starts the snapshot");

#endif //LAB1_FLAGS_H
