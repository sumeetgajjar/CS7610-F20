#include <iostream>
#include <glog/logging.h>
#include <gflags/gflags.h>

DEFINE_int32(test, 1, "dummy");

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    LOG(INFO) << "Flag -> " << FLAGS_test;
    return 0;
}
