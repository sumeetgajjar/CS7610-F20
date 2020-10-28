//
// Created by sumeet on 10/15/20.
//

#ifndef LAB2_MEMBERSHIP_H
#define LAB2_MEMBERSHIP_H

#include <unordered_map>
#include <vector>
#include <bits/unordered_set.h>
#include <set>
#include <mutex>

#include "message.h"
#include "network_utils.h"

#define HEARTBEAT_INTERVAL_MS 1000

namespace lab2 {
    typedef std::unordered_map<std::string, PeerId> HostnameToPeerIdMap;
    typedef std::unordered_map<PeerId, std::string> PeerIdToHostnameMap;
    typedef std::unordered_map<PeerId, TcpClient> TcpClientMap;

    class MembershipService {
        const int membershipPort;
        const int heartBeatPort;
        const std::vector<std::string> allPeerHostnames;

        PeerId myPeerId;
        HostnameToPeerIdMap hostnameToPeerIdMap;
        PeerIdToHostnameMap peerIdToHostnameMap;

        PeerId leaderPeerId = 1;
        RequestId requestIdCounter = 1;
        ViewId viewId = 1;
        std::recursive_mutex alivePeersMutex;
        std::set<PeerId> alivePeers;
        TcpClientMap tcpClientMap;
        RequestMsg pendingRequest;

        std::set<PeerId> getGroupMembers();

        RequestMsg createRequestMsg(PeerId newPeerId, OperationTypeEnum operationTypeEnum);

        OkMsg createOkMsg() const;

        void sendRequestMsg(PeerId peerId, const RequestMsg &requestMsg);

        void sendOkMsg(const OkMsg &okMsg);

        void sendNewViewMsg();

        void waitForRequestMsg();

        void waitForNewViewMsg();

        void waitForOkMsg(PeerId peerId, RequestId expectedRequestId);

        void modifyGroupMembership(PeerId peerId, OperationTypeEnum operationType);

        [[noreturn]] void startListening();

        [[noreturn]] void connectToLeader();

        void printNewlyInstalledView();

        void startSendingHeartBeat();

    public:

        MembershipService(int membershipPort, int heartBeatPort,
                          std::vector<std::string> allPeerHostnames_);

        void start();
    };
}
#endif //LAB2_MEMBERSHIP_H
