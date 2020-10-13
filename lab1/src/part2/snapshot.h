//
// Created by sumeet on 10/10/20.
//

#ifndef LAB1_SNAPSHOT_H
#define LAB1_SNAPSHOT_H

#include <vector>
#include <functional>
#include "../common/network_utils.h"
#include "../common/message.h"

#define SNAPSHOT_PORT 10002

namespace lab1 {

    class IncomingChannelState {
        std::vector<std::string> messages;
    public:
        void recordMessage(const Message &message);

        const std::vector<std::string> &getRecordedMessages() const;
    };

    std::ostream &operator<<(std::ostream &o, const IncomingChannelState &incomingChannelState);

    class SnapshotService {
        const uint32_t senderId;
        const std::vector<std::string> allPeers;
        TcpServer tcpServer;

        std::function<std::string()> localStateGetter;
        std::unordered_map<std::string, IncomingChannelState> incomingChannels;
        std::string localState;

        std::mutex channelsToBeRecordedMutex;
        std::unordered_set<std::string> channelsToBeRecorded;

        std::mutex snapshotInitiatedMutex;
        bool snapshotInitiated;

        void sendMarkerMessageToPeers();

        void takeSnapshot(const std::vector<std::string> &channelsToRecord);

        MarkerMessage createMarkerMessage() const;

        void printSnapshot() const;

    public:
        explicit SnapshotService(uint32_t senderId, const std::vector<std::string> &peers);

        void setLocalStateGetter(std::function<std::string(void)> getter);

        void recordIncomingMessages(const Message &message);

        void takeSnapshot();

        void start();
    };
}

#endif //LAB1_SNAPSHOT_H
