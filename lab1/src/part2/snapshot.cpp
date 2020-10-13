//
// Created by sumeet on 10/10/20.
//

#include <glog/logging.h>
#include <thread>
#include <utility>
#include <deque>
#include <future>

#include "snapshot.h"

#include "../common/serde.h"
#include "../common/utils.h"

namespace lab1 {

    void IncomingChannelState::recordMessage(const Message &message) {
        auto msgType = Serde::getMessageType(message);
        VLOG(1) << "recording msgType: " << msgType << " from sender: " << message.sender;
        auto msgStr = [&]() {
            switch (msgType) {
                case MessageType::Data: {
                    std::stringstream ss;
                    ss << Serde::deserializeDataMsg(message);
                    return ss.str();
                }
                case MessageType::Ack: {
                    std::stringstream ss;
                    ss << Serde::deserializeAckMessage(message);
                    return ss.str();
                }
                case MessageType::Seq: {
                    std::stringstream ss;
                    ss << Serde::deserializeSeqMessage(message);
                    return ss.str();
                }
                case MessageType::SeqAck: {
                    std::stringstream ss;
                    ss << Serde::deserializeSeqAckMessage(message);
                    return ss.str();
                }
                default: {
                    std::string errMsg("unknown message type: " + std::to_string(msgType) + ", from:" + message.sender);
                    LOG(ERROR) << errMsg;
                    return errMsg;
                }
            }
        }();
        messages.push_back(msgStr);
        LOG(INFO) << "recorded msgType: " << msgType << " from sender: " << message.sender << ", message: " << msgStr;
    }

    const std::vector<std::string> &IncomingChannelState::getRecordedMessages() const {
        return messages;
    }

    SnapshotService::SnapshotService(uint32_t senderId, const std::vector<std::string> &peers)
            : senderId(senderId),
              allPeers(peers),
              tcpServer(SNAPSHOT_PORT),
              incomingChannels(peers.size()),
              snapshotInitiated(false) {}

    void SnapshotService::sendMarkerMessageToPeers() {
        char buffer[sizeof(MarkerMessage)];
        const MarkerMessage &markerMsg = createMarkerMessage();
        Serde::serializeMarkerMessage(markerMsg, buffer);

        for (const auto &peer : allPeers) {
            VLOG(1) << "sending marker message to " << peer << ", message: " << markerMsg;
            auto tcpClient = TcpClient(peer, SNAPSHOT_PORT);
            tcpClient.send(buffer, sizeof(buffer));
            tcpClient.close();
            LOG(INFO) << "marker message sent to " << peer << ", message: " << markerMsg;
        }
    }

    MarkerMessage SnapshotService::createMarkerMessage() const {
        MarkerMessage msg;
        msg.type = MessageType::Marker;
        msg.sender = senderId;
        return msg;
    }

    void SnapshotService::takeSnapshot() {
        LOG(INFO) << "initiating global snapshot";
        {
            std::lock_guard<std::mutex> lockGuard(snapshotInitiatedMutex);
            snapshotInitiated = true;
        }
        takeSnapshot(allPeers);
    }

    void SnapshotService::takeSnapshot(const std::vector<std::string> &channelsToRecord) {
        LOG(INFO) << "starting local snapshot";
        std::lock_guard<std::mutex> lockGuard(channelsToBeRecordedMutex);
        LOG(INFO) << "recording local state";
        localState = localStateGetter();
        LOG(INFO) << "local state recorded";
        for (const auto &channel : channelsToRecord) {
            LOG(INFO) << "starting recording on channel: " << channel;
            channelsToBeRecorded.insert(channel);
            incomingChannels[channel] = IncomingChannelState();
        }
        sendMarkerMessageToPeers();
    }

    void SnapshotService::start() {
        LOG(INFO) << "starting snapshot service";
        std::vector<std::thread> ioThreads;
        for (int i = 0; i < allPeers.size(); ++i) {
            auto client = tcpServer.accept();
            ioThreads.emplace_back([&](TcpClient client) {
                auto msg = client.receive();
                MessageType msgType = Serde::getMessageType(msg);
                const auto sender = msg.getParsedSender();
                CHECK(msgType == MessageType::Marker) << "unknown message type: " << msgType
                                                      << ", received from: " << sender;

                MarkerMessage markerMsg = Serde::deserializeMarkerMessage(msg);
                {
                    std::lock_guard<std::mutex> lockGuard(snapshotInitiatedMutex);
                    if (!snapshotInitiated) {
                        LOG(INFO) << "first marker message received from: " << sender << ", markerMsg: " << markerMsg;
                        std::vector<std::string> remainingPeers(allPeers.size() - 1);
                        std::copy_if(allPeers.begin(), allPeers.end(), remainingPeers.begin(),
                                     [&](const std::string &peer) { return peer != sender; });
                        takeSnapshot(remainingPeers);
                        snapshotInitiated = true;
                    }
                }

                LOG(INFO) << "received marker from: " << msg.sender << ", markerMsg: " << markerMsg;
                {
                    std::lock_guard<std::mutex> lockGuard(channelsToBeRecordedMutex);
                    channelsToBeRecorded.erase(sender);
                }
                LOG(INFO) << "stopped recording on channel: " << sender;
            }, client);
        }

        for (auto &thread : ioThreads) {
            thread.join();
        }
        tcpServer.close();
        printSnapshot();
    }

    void SnapshotService::recordIncomingMessages(const Message &message) {
        auto sender = message.getParsedSender();
        VLOG(1) << "inside recordIncomingMessages, sender: " << sender << ", bytes: " << message.n;
        std::lock_guard<std::mutex> lockGuard(channelsToBeRecordedMutex);
        if (channelsToBeRecorded.find(sender) != channelsToBeRecorded.end()) {
            incomingChannels.at(sender).recordMessage(message);
        }
    }

    void SnapshotService::setLocalStateGetter(std::function<std::string()> getter) {
        VLOG(1) << "setting localStateGetter";
        this->localStateGetter = std::move(getter);
    }

    void SnapshotService::printSnapshot() const {
        LOG(INFO) << "snapshot algorithm completed";

        std::stringstream ss;
        ss << "\n=================================== start of snapshot ===================================\n"
           << "\n=================================== start of localState ===================================\n"
           << "\n" << localState << "\n"
           << "\n=================================== end of localState ===================================\n";

        for (const auto &pair : incomingChannels) {
            ss << "\n======================= start of state for " << pair.first << " channel =======================\n"
               << "\n" << pair.second << "\n"
               << "\n======================== end of state for " << pair.first << " channel ========================\n";
        }

        ss << "\n=================================== end of snapshot ===================================\n";

        LOG(INFO) << ss.str();
    }

    std::ostream &operator<<(std::ostream &o, const IncomingChannelState &incomingChannelState) {
        for (const auto &msg: incomingChannelState.getRecordedMessages()) {
            o << msg << "\n";
        }
        return o;
    }
}