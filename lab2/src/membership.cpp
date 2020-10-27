//
// Created by sumeet on 10/15/20.
//

#include "membership.h"
#include "serde.h"
#include "utils.h"
#include <glog/logging.h>

#include <utility>
#include <algorithm>
#include <future>

namespace lab2 {

    RequestMsg getEmptyPendingRequestMsg() {
        RequestMsg requestMsg;
        requestMsg.msgType = MsgTypeEnum::REQUEST;
        requestMsg.requestId = 0;
        requestMsg.currentViewId = 0;
        requestMsg.operationType = OperationTypeEnum::NOTHING;
        requestMsg.peerId = 0;
        return requestMsg;
    }

    bool isEmptyPendingRequestMsg(const RequestMsg &requestMsg) {
        return requestMsg.msgType == MsgTypeEnum::REQUEST &&
               requestMsg.requestId == 0 &&
               requestMsg.currentViewId == 0 &&
               requestMsg.operationType == OperationTypeEnum::NOTHING &&
               requestMsg.peerId == 0;
    }

    void MembershipService::addPeerToMembershipList(PeerId newPeerId) {
        VLOG(1) << "inside addPeerToMembershipList, peerId: " << newPeerId;
        RequestMsg requestMsg = createRequestMsg(newPeerId);
        for (const auto peerId: groupMembers) {
            if (conflict(peerId)) {
                continue;
            }
            sendRequestMsg(peerId, requestMsg);
            auto okMsg = waitForOkMsg(peerId);
            CHECK_EQ(okMsg.requestId, requestMsg.requestId);
        }

        viewId++;
        groupMembers.insert(newPeerId);
        LOG(INFO) << "added peerId: " << newPeerId << " to the group" << ", GroupSize: " << groupMembers.size();
        printNewlyInstalledView();
        sendNewViewMsg();
    }

    RequestMsg MembershipService::createRequestMsg(PeerId newPeerId) {
        VLOG(1) << "creating RequestMsg";
        RequestMsg requestMsg;
        requestMsg.msgType = MsgTypeEnum::REQUEST;
        requestMsg.requestId = ++requestIdCounter;
        requestMsg.currentViewId = viewId;
        requestMsg.operationType = ADD;
        requestMsg.peerId = newPeerId;
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

        pendingRequest = getEmptyPendingRequestMsg();
    }

    void MembershipService::start() {
        if (myPeerId == leaderPeerId) {
            startListening();
        } else {
            connectToLeader();
        }
    }

    [[noreturn]] void MembershipService::startListening() {
        LOG(INFO) << "starting MembershipService on port: " << membershipPort;
        groupMembers.insert(myPeerId);
        printNewlyInstalledView();

        TcpServer server(membershipPort);
        while (true) {
            TcpClient tcpClient = server.accept();
            PeerId newPeerId = hostnameToPeerIdMap.at(tcpClient.getHostname());
            LOG(INFO) << "new peer detected, peerId: " << newPeerId << ", hostname: " << tcpClient.getHostname();
            tcpClientMap.insert(std::make_pair(newPeerId, tcpClient));
            addPeerToMembershipList(newPeerId);
        }
    }

    [[noreturn]] void MembershipService::connectToLeader() {
        auto leaderHostname = peerIdToHostnameMap.at(leaderPeerId);
        LOG(INFO) << "connecting to leader, peerId: " << leaderPeerId << ", host: " << leaderHostname;
        auto leaderTcpClient = TcpClient(leaderHostname, membershipPort);
        tcpClientMap.insert(std::make_pair(leaderPeerId, leaderTcpClient));
        while (true) {
            waitForNewViewMsg();
            waitForAddRequestMsg();
            sendOkMsg(createOkMsg());
        }
    }

    void MembershipService::waitForNewViewMsg() {
        TcpClient leaderTcpClient = tcpClientMap.at(leaderPeerId);
        LOG(INFO) << "waiting for NewViewMsg from leader: " << leaderPeerId;
        auto rawNewViewMessage = leaderTcpClient.receive();
        checkIfPeerCrashed(leaderPeerId, rawNewViewMessage);

        auto msgTypeEnum = SerDe::getMsgType(rawNewViewMessage);
        LOG(INFO) << "received " << msgTypeEnum << " from leaderPeerId: " << leaderPeerId;
        CHECK_EQ(msgTypeEnum, MsgTypeEnum::NEW_VIEW);

        auto newViewMsg = SerDe::deserializeNewViewMsg(rawNewViewMessage);
        LOG(INFO) << "received newViewMsg: " << newViewMsg << ", from leader: " << leaderPeerId;
        viewId = newViewMsg.newViewId;

        groupMembers.clear();
        for (int i = 0; i < newViewMsg.numberOfMembers; i++) {
            groupMembers.insert(newViewMsg.members[i]);
        }
        printNewlyInstalledView();
    }

    void MembershipService::waitForAddRequestMsg() {
        TcpClient leaderTcpClient = tcpClientMap.at(leaderPeerId);
        LOG(INFO) << "waiting for RequestMsg from leader: " << leaderPeerId;
        auto rawReqMessage = leaderTcpClient.receive();
        checkIfPeerCrashed(leaderPeerId, rawReqMessage);

        auto msgTypeEnum = SerDe::getMsgType(rawReqMessage);
        LOG(INFO) << "received " << msgTypeEnum << " from leaderPeerId: " << leaderPeerId;
        CHECK_EQ(msgTypeEnum, MsgTypeEnum::REQUEST);

        pendingRequest = SerDe::deserializeRequestMsg(rawReqMessage);
        LOG(INFO) << "received requestMsg: " << pendingRequest << ", from leader: " << leaderPeerId;
        CHECK_EQ(pendingRequest.operationType, OperationTypeEnum::ADD);
    }

    void MembershipService::sendRequestMsg(PeerId peerId, const RequestMsg &requestMsg) {
        LOG(INFO) << "sending RequestMsg: " << requestMsg << ", to peerId: " << peerId;
        char buffer[sizeof(RequestMsg)];
        SerDe::serializeRequestMsg(requestMsg, buffer);

        auto tcpClient = tcpClientMap.at(peerId);
        tcpClient.send(buffer, sizeof(RequestMsg));
        VLOG(1) << "requestMsg sent to peerId: " << peerId;
    }

    void MembershipService::sendOkMsg(const OkMsg &okMsg) {
        LOG(INFO) << "sending OkMsg: " << okMsg;
        char buffer[sizeof(OkMsg)];
        SerDe::serializeOkMsg(okMsg, buffer);

        auto tcpClient = tcpClientMap.at(leaderPeerId);
        tcpClient.send(buffer, sizeof(OkMsg));
    }

    OkMsg MembershipService::createOkMsg() const {
        VLOG(1) << "creating OkMsg";
        OkMsg okMsg;
        okMsg.msgType = MsgTypeEnum::OK;
        okMsg.currentViewId = viewId;
        okMsg.requestId = pendingRequest.requestId;
        return okMsg;
    }

    void MembershipService::sendNewViewMsg() {
        NewViewMsg newViewMsg;
        newViewMsg.msgType = MsgTypeEnum::NEW_VIEW;
        newViewMsg.newViewId = viewId;
        newViewMsg.numberOfMembers = groupMembers.size();
        int i = 0;
        for (const auto &peer : groupMembers) {
            newViewMsg.members[i++] = peer;
        }

        LOG(INFO) << "sending NewViewMsg: " << newViewMsg;
        for (const auto &peerId : groupMembers) {
            if (conflict(peerId)) {
                continue;
            }

            char buffer[sizeof(NewViewMsg)];
            SerDe::serializeNewViewMsg(newViewMsg, buffer);

            auto tcpClient = tcpClientMap.at(peerId);
            tcpClient.send(buffer, sizeof(NewViewMsg));
        }
        VLOG(1) << "newViewMsg sent to all peers";
    }

    void MembershipService::handleLeaderCrash() {

    }

    void MembershipService::checkIfPeerCrashed(const PeerId peerId, const Message &message) {
        if (message.n == 0) {
            throw PeerCrashedException("peerId: " + std::to_string(peerId) + " crashed");
        }
    }

    void MembershipService::printNewlyInstalledView() {
        std::stringstream ss;
        ss << "{";
        int i = 0;
        for (const auto peer: groupMembers) {
            if (i != 0) {
                ss << ", ";
            }
            ss << peer;
            i++;
        }
        ss << "}";
        LOG(INFO) << "new view installed, viewId: " << viewId << ", members: " << ss.str();
    }

    OkMsg MembershipService::waitForOkMsg(const PeerId peerId) {
        LOG(INFO) << "waiting for OkMsg from peerId: " << peerId;
        auto tcpClient = tcpClientMap.at(peerId);
        auto rawOkMessage = tcpClient.receive();
        checkIfPeerCrashed(peerId, rawOkMessage);

        auto msgTypeEnum = SerDe::getMsgType(rawOkMessage);
        LOG(INFO) << "received " << msgTypeEnum << " from peerId: " << peerId;
        CHECK_EQ(msgTypeEnum, MsgTypeEnum::OK);

        auto okMsg = SerDe::deserializeOkMsg(rawOkMessage);
        LOG(INFO) << "received okMsg: " << okMsg << ", from peerId: " << peerId;
        return okMsg;
    }

    bool MembershipService::conflict(PeerId peerId) const {
        return peerId == myPeerId;
    }

    PeerCrashedException::PeerCrashedException(const std::string &message) : std::runtime_error(message) {}
}

//TODO add logging