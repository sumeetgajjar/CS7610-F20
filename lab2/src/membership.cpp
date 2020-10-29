//
// Created by sumeet on 10/15/20.
//

#include "membership.h"
#include "serde.h"
#include "utils.h"
#include <glog/logging.h>
#include <thread>

namespace lab2 {

    RequestMsg getDummyPendingRequestMsg() {
        RequestMsg requestMsg;
        requestMsg.msgType = MsgTypeEnum::REQUEST;
        requestMsg.requestId = 0;
        requestMsg.currentViewId = 0;
        requestMsg.operationType = OperationTypeEnum::NOTHING;
        requestMsg.peerId = 0;
        return requestMsg;
    }

    MembershipService::MembershipService(const int membershipPort, const int heartBeatPort,
                                         std::vector<std::string> allPeerHostnames_)
            : membershipPort(membershipPort),
              heartBeatPort(heartBeatPort),
              allPeerHostnames(std::move(allPeerHostnames_)) {

        for (const auto &hostname : allPeerHostnames) {
            auto peerId = Utils::getProcessIdentifier(hostname, allPeerHostnames);
            hostnameToPeerIdMap[hostname] = peerId;
            peerIdToHostnameMap[peerId] = hostname;
        }

        auto currentHostname = NetworkUtils::getCurrentHostname();
        myPeerId = hostnameToPeerIdMap.at(currentHostname);
        LOG(INFO) << "myPeerId: " << myPeerId;

        pendingRequest = getDummyPendingRequestMsg();
    }

    std::set<PeerId> MembershipService::getGroupMembers() {
        std::scoped_lock<std::recursive_mutex> lock(alivePeersMutex);
        std::set<PeerId> groupMembers(alivePeers.begin(), alivePeers.end());
        groupMembers.insert(myPeerId);
        return groupMembers;
    }

    RequestMsg MembershipService::createRequestMsg(PeerId newPeerId, OperationTypeEnum operationTypeEnum) {
        VLOG(1) << "creating RequestMsg for " << operationTypeEnum;
        RequestMsg requestMsg;
        requestMsg.msgType = MsgTypeEnum::REQUEST;
        requestMsg.requestId = ++requestIdCounter;
        requestMsg.currentViewId = viewId;
        requestMsg.operationType = operationTypeEnum;
        requestMsg.peerId = newPeerId;
        return requestMsg;
    }

    OkMsg MembershipService::createOkMsg() const {
        VLOG(1) << "creating OkMsg";
        OkMsg okMsg;
        okMsg.msgType = MsgTypeEnum::OK;
        okMsg.currentViewId = viewId;
        okMsg.requestId = pendingRequest.requestId;
        return okMsg;
    }

    NewLeaderMsg MembershipService::createNewLeaderMsg() {
        NewLeaderMsg msg;
        msg.msgType = MsgTypeEnum::NEW_LEADER;
        msg.requestId = ++requestIdCounter;
        msg.currentViewId = viewId;
        msg.operationType = OperationTypeEnum::PENDING;
        return msg;
    }

    void MembershipService::sendRequestMsg(const RequestMsg &requestMsg) {
        LOG(INFO) << "sending RequestMsg: " << requestMsg;
        char buffer[sizeof(RequestMsg)];
        SerDe::serializeRequestMsg(requestMsg, buffer);
        std::scoped_lock<std::recursive_mutex> lock(alivePeersMutex);
        bool isTestCase4 = FLAGS_leaderFailureDemo && requestMsg.operationType == OperationTypeEnum::DEL;
        for (const auto peerId: alivePeers) {
            // Special if stmt to demo TestCase 4
            if (isTestCase4 && peerId == 2) {
                continue;
            }
            auto tcpClient = tcpClientMap.at(peerId);
            tcpClient.send(buffer, sizeof(RequestMsg));
        }
        VLOG(1) << "requestMsg sent";
        if (isTestCase4) {
            LOG(WARNING) << "crashing Leader purposefully for TestCase 4";
            exit(0);
        }
    }

    void MembershipService::sendOkMsg(const OkMsg &okMsg) {
        LOG(INFO) << "sending OkMsg: " << okMsg;
        char buffer[sizeof(OkMsg)];
        SerDe::serializeOkMsg(okMsg, buffer);

        auto tcpClient = tcpClientMap.at(leaderPeerId);
        tcpClient.send(buffer, sizeof(OkMsg));
    }

    void MembershipService::sendNewViewMsg() {
        NewViewMsg newViewMsg;
        newViewMsg.msgType = MsgTypeEnum::NEW_VIEW;
        newViewMsg.newViewId = viewId;
        {
            std::scoped_lock<std::recursive_mutex> lock(alivePeersMutex);
            std::set<PeerId> members = getGroupMembers();
            newViewMsg.numberOfMembers = members.size();
            int i = 0;
            for (const auto &peer : members) {
                newViewMsg.members[i++] = peer;
            }

            LOG(INFO) << "sending NewViewMsg: " << newViewMsg;
            for (const auto &peerId : alivePeers) {
                char buffer[sizeof(NewViewMsg)];
                SerDe::serializeNewViewMsg(newViewMsg, buffer);

                auto tcpClient = tcpClientMap.at(peerId);
                tcpClient.send(buffer, sizeof(NewViewMsg));
            }
        }
        LOG(INFO) << "newViewMsg delivered to all peers";
    }

    void MembershipService::sendPendingRequestMsg() {
        VLOG(1) << "inside sendPendingRequestMsg";
        auto requestMsg = pendingRequest;
        requestMsg.currentViewId = viewId;
        LOG(INFO) << "sending pendingRequestMsg: " << pendingRequest << ", to leaderPeerId: " << leaderPeerId;
        char buffer[sizeof(RequestMsg)];
        SerDe::serializeRequestMsg(requestMsg, buffer);
        auto leaderTcpClient = tcpClientMap.at(leaderPeerId);
        leaderTcpClient.send(buffer, sizeof(RequestMsg));
    }

    void MembershipService::sendNewLeaderMsg() {
        VLOG(1) << "inside sendNewLeaderMsg";
        auto newLeaderMsg = createNewLeaderMsg();
        LOG(INFO) << "sending newLeaderMsg: " << newLeaderMsg;
        char buffer[sizeof(NewLeaderMsg)];
        SerDe::serializeNewLeaderMsg(newLeaderMsg, buffer);
        for (const auto &peer : alivePeers) {
            try {
                auto client = TcpClient(peerIdToHostnameMap.at(peer), membershipPort, 5);
                client.send(buffer, sizeof(NewLeaderMsg));
                tcpClientMap.insert(std::make_pair(peer, client));
            } catch (const std::runtime_error &e) {
                LOG(ERROR) << "cannot send newLeaderMsg to peerId: " << peer;
            }
        }
    }

    void MembershipService::waitForRequestMsg() {
        TcpClient leaderTcpClient = tcpClientMap.at(leaderPeerId);
        VLOG(1) << "waiting for RequestMsg from leader: " << leaderPeerId;
        auto rawReqMessage = leaderTcpClient.receive();

        auto msgTypeEnum = SerDe::getMsgType(rawReqMessage);
        VLOG(1) << "received " << msgTypeEnum << " from leaderPeerId: " << leaderPeerId;
        CHECK_EQ(msgTypeEnum, MsgTypeEnum::REQUEST);
        pendingRequest = SerDe::deserializeRequestMsg(rawReqMessage);
        LOG(INFO) << "received requestMsg: " << pendingRequest << ", from leader: " << leaderPeerId;
    }

    void MembershipService::waitForOkMsg(RequestId expectedRequestId) {
        std::scoped_lock<std::recursive_mutex> lock(alivePeersMutex);
        for (const auto peerId: alivePeers) {
            VLOG(1) << "waiting for OkMsg from peerId: " << peerId;
            auto tcpClient = tcpClientMap.at(peerId);
            auto rawOkMessage = tcpClient.receive();

            auto msgTypeEnum = SerDe::getMsgType(rawOkMessage);
            VLOG(1) << "received " << msgTypeEnum << " from peerId: " << peerId;
            CHECK_EQ(msgTypeEnum, MsgTypeEnum::OK);
            auto okMsg = SerDe::deserializeOkMsg(rawOkMessage);
            LOG(INFO) << "received okMsg: " << okMsg << ", from peerId: " << peerId;
            CHECK_EQ(okMsg.requestId, expectedRequestId);
        }
    }

    void MembershipService::waitForNewViewMsg() {
        TcpClient leaderTcpClient = tcpClientMap.at(leaderPeerId);
        VLOG(1) << "waiting for NewViewMsg from leader: " << leaderPeerId;
        auto rawNewViewMessage = leaderTcpClient.receive();

        auto msgTypeEnum = SerDe::getMsgType(rawNewViewMessage);
        VLOG(1) << "received " << msgTypeEnum << " from leaderPeerId: " << leaderPeerId;
        CHECK_EQ(msgTypeEnum, MsgTypeEnum::NEW_VIEW);

        auto newViewMsg = SerDe::deserializeNewViewMsg(rawNewViewMessage);
        LOG(INFO) << "received newViewMsg: " << newViewMsg << ", from leader: " << leaderPeerId;
        viewId = newViewMsg.newViewId;

        {
            std::scoped_lock<std::recursive_mutex> lock(alivePeersMutex);
            alivePeers.clear();
            for (int i = 0; i < newViewMsg.numberOfMembers; i++) {
                alivePeers.insert(newViewMsg.members[i]);
            }
            alivePeers.erase(myPeerId);
        }
        pendingRequest = getDummyPendingRequestMsg();
        printNewlyInstalledView();
    }

    void MembershipService::waitForNewLeaderMsg() {
        VLOG(1) << "inside waitForNewLeaderMsg";
        TcpServer tcpServer(membershipPort);
        VLOG(1) << "waiting for newLeaderMsg";
        auto leaderClient = tcpServer.accept();
        tcpServer.close();

        leaderPeerId = hostnameToPeerIdMap.at(leaderClient.getHostname());
        tcpClientMap.insert(std::make_pair(leaderPeerId, leaderClient));
        LOG(INFO) << "new leader detected, peerId: " << leaderPeerId;

        auto message = leaderClient.receive();
        auto msgType = SerDe::getMsgType(message);
        CHECK_EQ(msgType, MsgTypeEnum::NEW_LEADER);
        auto newLeaderMsg = SerDe::deserializeNewLeaderMsg(message);
        LOG(INFO) << "received newLeaderMsg: " << newLeaderMsg << ", from leaderPeerId: " << leaderPeerId;
    }

    RequestMsg MembershipService::waitForPendingRequestMsg() {
        VLOG(1) << "inside waitForPendingRequestMsg";
        std::scoped_lock<std::recursive_mutex> lock(alivePeersMutex);
        RequestMsg pendingRequestMsg = getDummyPendingRequestMsg();
        for (const auto &peer : alivePeers) {
            if (tcpClientMap.find(peer) != tcpClientMap.end()) {
                auto tcpClient = tcpClientMap.at(peer);
                VLOG(1) << "waiting for pendingRequestMsg from peerId: " << peer;
                auto message = tcpClient.receive();
                auto msgType = SerDe::getMsgType(message);
                CHECK_EQ(msgType, MsgTypeEnum::REQUEST);
                auto requestMsg = SerDe::deserializeRequestMsg(message);
                VLOG(1) << "received pendingRequestMsg: " << requestMsg << ", from peerId:" << peer;
                if (requestMsg.operationType != OperationTypeEnum::NOTHING) {
                    LOG(INFO) << "received pendingRequestMsg: " << requestMsg << ", from peerId:" << peer;
                    pendingRequestMsg = requestMsg;
                }
            }
        }
        return pendingRequestMsg;
    }

    void MembershipService::modifyGroupMembership(PeerId peerId, OperationTypeEnum operationType) {
        VLOG(1) << "inside modifyGroupMembership, peerId: " << peerId << ", operationType: " << operationType;
        RequestMsg requestMsg = createRequestMsg(peerId, operationType);
        {
            std::scoped_lock<std::recursive_mutex> lock(alivePeersMutex);

            // removing the peer from the alive list since it is not reachable
            if (operationType == OperationTypeEnum::DEL) {
                tcpClientMap.erase(peerId);
                alivePeers.erase(peerId);
                LOG(INFO) << "removed peerId: " << peerId << " from the group"
                          << ", GroupSize: " << getGroupMembers().size();
            }

            sendRequestMsg(requestMsg);
            waitForOkMsg(requestMsg.requestId);

            if (operationType == OperationTypeEnum::ADD) {
                alivePeers.insert(peerId);
                LOG(INFO) << "added peerId: " << peerId << " to the group"
                          << ", GroupSize: " << getGroupMembers().size();
            }
            viewId++;
        }
        printNewlyInstalledView();
        sendNewViewMsg();
    }

    void MembershipService::handleLeaderCrash() {
        VLOG(1) << "inside handleLeaderCrash";
        tcpClientMap.erase(leaderPeerId);
        auto nextLeader = [&]() {
            for (const auto peer : getGroupMembers()) {
                if (peer != leaderPeerId) {
                    return peer;
                }
            }
            return (PeerId) 0;
        }();
        CHECK_NE(nextLeader, 0);
        LOG(INFO) << "new leader candidate peerId: " << nextLeader;
        if (myPeerId == nextLeader) {
            LOG(INFO) << "I am the new leader";
            {
                std::scoped_lock<std::recursive_mutex> lock(alivePeersMutex);
                alivePeers.erase(leaderPeerId);
            }
            sendNewLeaderMsg();
            auto pendingRequestMsg = waitForPendingRequestMsg();
            if (pendingRequestMsg.operationType != NOTHING) {
                LOG(INFO) << "completing pending request: " << pendingRequestMsg;
                modifyGroupMembership(pendingRequestMsg.peerId,
                                      static_cast<OperationTypeEnum>(pendingRequestMsg.operationType));
            } else {
                sendNewViewMsg();
            }
            leaderPeerId = nextLeader;
            startListening();
        } else {
            waitForNewLeaderMsg();
            sendPendingRequestMsg();
            waitForRequestMsg();
            sendOkMsg(createOkMsg());
        }
    }

    [[noreturn]] void MembershipService::startListening() {
        LOG(INFO) << "starting listening for new peers on port: " << membershipPort;

        TcpServer server(membershipPort);
        while (true) {
            TcpClient tcpClient = server.accept();
            PeerId newPeerId = hostnameToPeerIdMap.at(tcpClient.getHostname());
            LOG(INFO) << "new peer detected, peerId: " << newPeerId << ", hostname: " << tcpClient.getHostname();
            tcpClientMap.insert(std::make_pair(newPeerId, tcpClient));
            modifyGroupMembership(newPeerId, OperationTypeEnum::ADD);
        }
    }

    [[noreturn]] void MembershipService::waitForMessagesFromLeader() {
        while (true) {
            waitForNewViewMsg();
            waitForRequestMsg();
            sendOkMsg(createOkMsg());
        }
    }

    void MembershipService::printNewlyInstalledView() {
        std::stringstream ss;
        ss << "{";
        int i = 0;
        for (const auto member: getGroupMembers()) {
            if (i != 0) {
                ss << ", ";
            }
            ss << member;
            i++;
        }
        ss << "}";
        LOG(INFO) << "installed view info, viewId: " << viewId << ", members: " << ss.str();
    }

    void MembershipService::startSendingHeartBeat() {
        VLOG(1) << "starting sending heartBeat";
        for (const auto &pair : hostnameToPeerIdMap) {
            if (pair.second == myPeerId) {
                continue;
            }

            std::thread([&]() {
                HeartBeatMsg msg;
                msg.msgType = MsgTypeEnum::HEARTBEAT;
                msg.peerId = myPeerId;
                char buffer[sizeof(HeartBeatMsg)];
                SerDe::serializeHeartBeatMsg(msg, buffer);

                while (true) {
                    auto hostname = pair.first;
                    VLOG(1) << "sending HeartBeatMsg to peerId: " << hostname << ", msg: " << msg;
                    try {
                        auto udpSender = UDPSender(hostname, heartBeatPort, 0);
                        udpSender.send(buffer, sizeof(HeartBeatMsg));
                        udpSender.close();
                    } catch (std::runtime_error &e) {
                        VLOG(1) << "exception occured while sending heartbeat msg to peerId: " << hostname;
                    }
                    VLOG(1) << "heartbeat sender for PeerId: " << pair.second
                            << "  sleeping for " << HEARTBEAT_INTERVAL_MS << " ms";
                    std::this_thread::sleep_for(std::chrono::milliseconds{HEARTBEAT_INTERVAL_MS});
                }
            }).detach();
        }
    }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

    void MembershipService::start() {
        startSendingHeartBeat();

        std::unordered_map<PeerId, int> peerHeartBeatMap;
        std::mutex peerHeartBeatMapMutex;

        std::thread heartBeatReceiverThread([&]() {
            VLOG(1) << "starting HeartBeatReceiver thread";
            UDPReceiver udpReceiver(heartBeatPort);
            while (true) {
                auto message = udpReceiver.receive();
                auto msgTypeEnum = SerDe::getMsgType(message);
                CHECK_EQ(msgTypeEnum, MsgTypeEnum::HEARTBEAT);
                VLOG(1) << "received " << msgTypeEnum << " from host: " << message.sender;
                auto heartBeatMsg = SerDe::deserializeHeartBeatMsg(message);
                VLOG(1) << "received HeartBeatMsg: " << heartBeatMsg;
                {
                    std::scoped_lock<std::mutex> lock(peerHeartBeatMapMutex);
                    peerHeartBeatMap[heartBeatMsg.peerId] = 2;
                }
            }
        });

        std::thread failureDetectorThread([&]() {
            VLOG(1) << "starting heartBeatMonitor Thread";
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
                            LOG(WARNING) << "Peer " << crashedPeerId << " not reachable";
                            if (myPeerId == leaderPeerId) {
                                modifyGroupMembership(crashedPeerId, OperationTypeEnum::DEL);
                            } else if (crashedPeerId == leaderPeerId) {
                                LOG(WARNING) << "Leader " << crashedPeerId << " has crashed";
                                {
                                    std::scoped_lock<std::mutex> lock(leaderCrashedMutex);
                                    leaderCrashed = true;
                                }
                                leaderCrashedCV.notify_one();
                            }
                            // breaking out of the loop since the iterator is invalidated due to modification
                            // of heartBeatReceivers set.
                            // iterating using invalidated iterator results in undefined behavior
                            break;
                        }
                    }
                }
            }
        });

        std::thread serviceThread([&]() {
            VLOG(1) << "starting service thread";
            if (myPeerId == leaderPeerId) {
                printNewlyInstalledView();
                startListening();
            } else {
                auto leaderHostname = peerIdToHostnameMap.at(leaderPeerId);
                LOG(INFO) << "connecting to leader, peerId: " << leaderPeerId << ", host: " << leaderHostname;
                auto leaderTcpClient = TcpClient(leaderHostname, membershipPort);
                tcpClientMap.insert(std::make_pair(leaderPeerId, leaderTcpClient));
                while (true) {
                    try {
                        waitForMessagesFromLeader();
                    } catch (const TransportException &e) {
                        VLOG(1) << "waiting for leader crash detection";
                        {
                            std::unique_lock<std::mutex> lock(leaderCrashedMutex);
                            leaderCrashedCV.wait(lock, [&] { return leaderCrashed; });
                            leaderCrashed = false;
                        }
                        handleLeaderCrash();
                    }
                }
            }
        });

        heartBeatReceiverThread.join();
        failureDetectorThread.join();
        serviceThread.join();
    }

#pragma clang diagnostic pop
}