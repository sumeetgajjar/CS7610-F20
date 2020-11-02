//
// Created by sumeet on 11/2/20.
//

#ifndef LAB2_FAILURE_DETECTOR_H
#define LAB2_FAILURE_DETECTOR_H

#include <functional>
#include <mutex>

#include "message.h"

#define HEARTBEAT_INTERVAL_MS 1000

namespace lab2 {
    class FailureDetector {
        const int heartBeatPort;
        const std::function<void(PeerId)> onFailureCallback;

        std::unordered_map<PeerId, int> peerHeartBeatMap;
        std::mutex peerHeartBeatMapMutex;

        [[noreturn]] void startHeartBeatSender(const std::string &hostname, PeerId peerId) const;

        [[noreturn]] void startDetectorThread();

        [[noreturn]] void startHeartBeatListener();

    public:
        FailureDetector(int heartBeatPort,
                        std::function<void(PeerId)> onFailureCallback);

        void start();
    };
}

#endif //LAB2_FAILURE_DETECTOR_H
