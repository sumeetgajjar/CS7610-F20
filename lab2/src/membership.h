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
#include <condition_variable>

#include <gflags/gflags.h>

#include "message.h"
#include "network_utils.h"

#define HEARTBEAT_INTERVAL_MS 1000
DECLARE_bool(leaderFailureDemo);

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

        bool leaderCrashed = false;
        std::mutex leaderCrashedMutex;
        std::condition_variable leaderCrashedCV;

        RequestMsg createRequestMsg(PeerId newPeerId, OperationTypeEnum operationTypeEnum);

        OkMsg createOkMsg() const;

        NewLeaderMsg createNewLeaderMsg();

        void sendRequestMsg(const RequestMsg &requestMsg);

        void sendOkMsg(const OkMsg &okMsg);

        void sendNewViewMsg();

        void sendPendingRequestMsg();

        void sendNewLeaderMsg();

        void waitForRequestMsg();

        void waitForOkMsg(RequestId expectedRequestId);

        void waitForNewViewMsg();

        void waitForNewLeaderMsg();

        RequestMsg waitForPendingRequestMsg();

        void modifyGroupMembership(PeerId peerId, OperationTypeEnum operationType);

        void findLeader();

        void handleLeaderCrash();

        [[noreturn]] void startListening();

        [[noreturn]] void waitForMessagesFromLeader();

        void printNewlyInstalledView();

        void startSendingHeartBeat();

    public:

        MembershipService(int membershipPort, int heartBeatPort,
                          std::vector<std::string> allPeerHostnames_);

        PeerId getMyPeerId();

        std::set<PeerId> getGroupMembers();

        void start();
    };
}
#endif //LAB2_MEMBERSHIP_H
