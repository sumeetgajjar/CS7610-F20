#include "socket_utils.h"

#include <glog/logging.h>
#include <climits>

namespace lab1 {
    std::string SocketUtils::getCurrentContainerHostname() {
        VLOG(1) << "getting current host";
        char hostname[HOST_NAME_MAX];
        int status = gethostname(hostname, HOST_NAME_MAX);
        CHECK(status == 0) << "cannot find the current host name, errorCode: " << errno;
        LOG(INFO) << "current host name: " << hostname;
        return std::string(hostname);
    }
}