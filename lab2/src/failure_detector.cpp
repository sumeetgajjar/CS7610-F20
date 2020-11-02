//
// Created by sumeet on 11/2/20.
//
#include <glog/logging.h>
#include <thread>
#include <utility>

#include "failure_detector.h"
#include "utils.h"
#include "network_utils.h"
#include "serde.h"

namespace lab2 {

    lab2::FailureDetector::FailureDetector(const int heartBeatPort,
                                           std::function<void(PeerId)> onFailureCallback) :
            heartBeatPort(heartBeatPort),
            onFailureCallback(std::move(onFailureCallback)) {}

    void FailureDetector::start() {
        std::thread heartBeatListenerThread([&]() {
            VLOG(1) << "starting HeartBeatReceiver thread";
            startHeartBeatListener();
        });

        for (const auto &hostname : PeerInfo::getAllPeerHostnames()) {
            auto peerId = PeerInfo::getPeerId(hostname);
            if (peerId == PeerInfo::getMyPeerId()) {
                continue;
            }

            std::thread([&]() {
                VLOG(1) << "starting sending heartBeat for peerId: " << peerId;
                startHeartBeatSender(hostname, peerId);
            }).detach();
        }

        std::thread failureDetectorThread([&]() {
            VLOG(1) << "starting heartBeat monitor Thread";
            startDetectorThread();
        });

        heartBeatListenerThread.join();
        failureDetectorThread.join();
    }

    [[noreturn]] void FailureDetector::startHeartBeatSender(const std::string &hostname, const PeerId peerId) const {
        HeartBeatMsg msg;
        msg.msgType = MsgTypeEnum::HEARTBEAT;
        msg.peerId = PeerInfo::getMyPeerId();
        char buffer[sizeof(HeartBeatMsg)];
        SerDe::serializeHeartBeatMsg(msg, buffer);

        while (true) {
            VLOG(1) << "sending HeartBeatMsg to peerId: " << hostname << ", msg: " << msg;
            try {
                auto udpSender = UDPSender(hostname, heartBeatPort, 0);
                udpSender.send(buffer, sizeof(HeartBeatMsg));
                udpSender.close();
            } catch (std::runtime_error &e) {
                VLOG(1) << "exception occured while sending heartbeat msg to peerId: " << hostname;
            }
            VLOG(1) << "heartbeat sender for PeerId: " << peerId
                    << "  sleeping for " << HEARTBEAT_INTERVAL_MS << " ms";
            std::this_thread::sleep_for(std::chrono::milliseconds{HEARTBEAT_INTERVAL_MS});
        }
    }

    [[noreturn]] void FailureDetector::startDetectorThread() {
        while (true) {
            VLOG(1) << "heartbeat monitor thread sleeping for " << HEARTBEAT_INTERVAL_MS << " ms";
            std::this_thread::sleep_for(std::chrono::milliseconds{HEARTBEAT_INTERVAL_MS});
            {
                std::scoped_lock<std::mutex> scopedLock(peerHeartBeatMapMutex);
                for (auto &pair : peerHeartBeatMap) {
                    VLOG(1) << "PeerId: " << pair.first << ", heartBeatCount: " << pair.second;
                    auto peerCrashed = pair.second == 0;
                    pair.second--;
                    if (peerCrashed) {
                        PeerId crashedPeerId = pair.first;
                        CHECK_EQ(peerHeartBeatMap.erase(crashedPeerId), 1);
                        LOG(WARNING) << "Peer: " << crashedPeerId << " is not reachable";
                        onFailureCallback(crashedPeerId);
                        // breaking out of the loop since the iterator is invalidated due to modification
                        // of heartBeatReceivers set.
                        // iterating using invalidated iterator results in undefined behavior
                        break;
                    }
                }
            }
        }
    }

    [[noreturn]] void FailureDetector::startHeartBeatListener() {
        UDPReceiver udpReceiver(heartBeatPort);
        while (true) {
            auto message = udpReceiver.receive();
            auto msgTypeEnum = SerDe::getMsgType(message);
            CHECK_EQ(msgTypeEnum, MsgTypeEnum::HEARTBEAT);
            const std::string &sender = message.getParsedSender();
            VLOG(1) << "received " << msgTypeEnum << " from host: " << sender;
            auto heartBeatMsg = SerDe::deserializeHeartBeatMsg(message);
            VLOG(1) << "received HeartBeatMsg: " << heartBeatMsg;
            CHECK_EQ(heartBeatMsg.peerId, PeerInfo::getPeerId(sender))
                << "peerId(" << heartBeatMsg.peerId << ") in HeartBeat msg does not match with senderPeerId("
                << PeerInfo::getPeerId(sender) << ")";
            {
                std::scoped_lock<std::mutex> lock(peerHeartBeatMapMutex);
                peerHeartBeatMap[heartBeatMsg.peerId] = 2;
            }
        }
    }
}