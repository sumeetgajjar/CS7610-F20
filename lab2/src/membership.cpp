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

    MembershipService::MembershipService(int membershipPort)
            : membershipPort(membershipPort), pendingRequest(getDummyPendingRequestMsg()) {}

    std::set<PeerId> MembershipService::getGroupMembers() {
        std::scoped_lock<std::recursive_mutex> lock(alivePeersMutex);
        std::set<PeerId> groupMembers(alivePeers.begin(), alivePeers.end());
        groupMembers.insert(PeerInfo::getMyPeerId());
        return groupMembers;
    }

    RequestMsg MembershipService::createRequestMsg(PeerId newPeerId, OperationTypeEnum operationTypeEnum) {
        VLOG(1) << "creating RequestMsg for " << operationTypeEnum;
        RequestMsg requestMsg;
        requestMsg.msgType = MsgTypeEnum::REQUEST;
        requestMsg.requestId = ++requestCounter;
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
        msg.requestId = ++requestCounter;
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
                auto client = TcpClient(PeerInfo::getHostname(peer), membershipPort, 5);
                client.send(buffer, sizeof(NewLeaderMsg));
                tcpClientMap.insert(std::make_pair(peer, client));
            } catch (const std::runtime_error &e) {
                LOG(ERROR) << "cannot send newLeaderMsg to peerId: " << peer;
            }
        }
    }

    void MembershipService::processRequestMsg(const Message &rawReqMessage) {
        VLOG(1) << "processing RequestMsg from leader: " << leaderPeerId;
        pendingRequest = SerDe::deserializeRequestMsg(rawReqMessage);
        LOG(INFO) << "received requestMsg: " << pendingRequest << ", from leader: " << leaderPeerId;
    }

    void MembershipService::processNewViewMsg(const Message &rawNewViewMessage) {
        VLOG(1) << "processing NewViewMsg from leader: " << leaderPeerId;
        auto newViewMsg = SerDe::deserializeNewViewMsg(rawNewViewMessage);
        LOG(INFO) << "received newViewMsg: " << newViewMsg << ", from leader: " << leaderPeerId;
        viewId = newViewMsg.newViewId;

        {
            std::scoped_lock<std::recursive_mutex> lock(alivePeersMutex);
            alivePeers.clear();
            for (int i = 0; i < newViewMsg.numberOfMembers; i++) {
                alivePeers.insert(newViewMsg.members[i]);
            }
            alivePeers.erase(PeerInfo::getMyPeerId());
        }
        pendingRequest = getDummyPendingRequestMsg();
        printNewlyInstalledView();
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

    void MembershipService::waitForNewLeaderMsg() {
        VLOG(1) << "inside waitForNewLeaderMsg";
        TcpServer tcpServer(membershipPort);
        VLOG(1) << "waiting for newLeaderMsg";
        auto leaderClient = tcpServer.accept();
        tcpServer.close();

        leaderPeerId = PeerInfo::getPeerId(leaderClient.getHostname());
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

    void MembershipService::findLeader() {
        for (const auto &hostname: PeerInfo::getAllPeerHostnames()) {
            auto peerId = PeerInfo::getPeerId(hostname);
            if (peerId == PeerInfo::getMyPeerId()) {
                leaderPeerId = peerId;
                return;
            }

            LOG(INFO) << "trying hostname: " << hostname << ", peerId: " << peerId << " as leader";
            try {
                auto leaderTcpClient = TcpClient(hostname, membershipPort, 5);
                leaderPeerId = peerId;
                tcpClientMap.insert(std::make_pair(leaderPeerId, leaderTcpClient));
                return;
            } catch (const std::runtime_error &e) {
                LOG(INFO) << "unable to connect to peerId: " << peerId;
            }
        }

        LOG(FATAL) << "no leader found";
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
        if (PeerInfo::getMyPeerId() == nextLeader) {
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
            startAsLeader();
        } else {
            waitForNewLeaderMsg();
            sendPendingRequestMsg();
        }
    }

    [[noreturn]] void MembershipService::startAsLeader() {
        LOG(INFO) << "starting listening for new peers on port: " << membershipPort;

        TcpServer server(membershipPort);
        while (true) {
            TcpClient tcpClient = server.accept();
            PeerId newPeerId = PeerInfo::getPeerId(tcpClient.getHostname());
            LOG(INFO) << "new peer detected, peerId: " << newPeerId << ", hostname: " << tcpClient.getHostname();
            tcpClientMap.insert(std::make_pair(newPeerId, tcpClient));
            modifyGroupMembership(newPeerId, OperationTypeEnum::ADD);
        }
    }

    [[noreturn]] void MembershipService::startAsFollower() {
        while (true) {
            TcpClient leaderTcpClient = tcpClientMap.at(leaderPeerId);
            VLOG(1) << "waiting for NewViewMsg from leader: " << leaderPeerId;
            auto message = leaderTcpClient.receive();

            auto msgTypeEnum = SerDe::getMsgType(message);
            VLOG(1) << "received " << msgTypeEnum << " from leaderPeerId: " << leaderPeerId;
            switch (msgTypeEnum) {
                case REQUEST:
                    processRequestMsg(message);
                    sendOkMsg(createOkMsg());
                    break;
                case NEW_VIEW:
                    processNewViewMsg(message);
                    break;
                default:
                    LOG(FATAL) << "unexpected messageType: " << msgTypeEnum
                               << " received from leaderPeerId: " << leaderPeerId;
            }
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

    void MembershipService::handlePeerFailure(PeerId crashedPeerId) {
        if (PeerInfo::getMyPeerId() == leaderPeerId) {
            modifyGroupMembership(crashedPeerId, OperationTypeEnum::DEL);
        } else if (crashedPeerId == leaderPeerId) {
            LOG(WARNING) << "Leader: " << crashedPeerId << " is not reachable";
            {
                std::scoped_lock<std::mutex> lock(leaderCrashedMutex);
                leaderCrashed = true;
            }
            leaderCrashedCV.notify_one();
        }
    }

    void MembershipService::start() {
        findLeader();
        LOG(INFO) << "leaderPeerId: " << leaderPeerId;
        if (PeerInfo::getMyPeerId() == leaderPeerId) {
            printNewlyInstalledView();
            startAsLeader();
        } else {
            while (true) {
                try {
                    startAsFollower();
                } catch (const TransportException &e) {
                    LOG(INFO) << "waiting for leader crash detection";
                    {
                        std::unique_lock<std::mutex> lock(leaderCrashedMutex);
                        leaderCrashedCV.wait(lock, [&] { return leaderCrashed; });
                        leaderCrashed = false;
                    }
                    handleLeaderCrash();
                }
            }
        }
    }
}