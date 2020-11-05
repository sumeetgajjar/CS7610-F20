//
// Created by sumeet on 11/2/20.
//

#ifndef LAB2_FAILURE_DETECTOR_H
#define LAB2_FAILURE_DETECTOR_H

#include <functional>
#include <mutex>
#include <set>

#include "message.h"

#define HEARTBEAT_INTERVAL_MS 1000

namespace lab2 {
    class FailureDetector {
        const int heartBeatPort;
        std::vector<std::function<void(PeerId)>> onFailureCallbacks;

        std::unordered_map<PeerId, int> peerHeartBeatMap;
        std::mutex peerHeartBeatMapMutex;

        [[noreturn]] void startHeartBeatSender(const std::string &hostname, PeerId peerId) const;

        [[noreturn]] void startDetectorThread();

        [[noreturn]] void startHeartBeatListener();

    public:
        explicit FailureDetector(int heartBeatPort);

        void start();

        void addPeerFailureCallback(const std::function<void(PeerId)> &callback);

        std::set<PeerId> getAlivePeers();
    };
}

#endif //LAB2_FAILURE_DETECTOR_H
