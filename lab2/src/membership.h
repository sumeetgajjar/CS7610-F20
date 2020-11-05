//
// Created by sumeet on 10/15/20.
//

#ifndef LAB2_MEMBERSHIP_H
#define LAB2_MEMBERSHIP_H

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <mutex>
#include <condition_variable>
#include <gflags/gflags.h>
#include <functional>

#include "message.h"
#include "network_utils.h"

DECLARE_bool(leaderFailureDemo);

namespace lab2 {
    typedef std::unordered_map<PeerId, TcpClient> TcpClientMap;

    class MembershipService {
        const int membershipPort;
        const std::function<std::set<PeerId>(void)> alivePeersGetter;

        PeerId leaderPeerId = 1;
        RequestId requestCounter = 1;
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

        void processRequestMsg(const Message &rawReqMessage);

        void processNewViewMsg(const Message &rawNewViewMessage);

        void waitForOkMsg(RequestId expectedRequestId);

        void waitForNewLeaderMsg();

        RequestMsg waitForPendingRequestMsg();

        void modifyGroupMembership(PeerId peerId, OperationTypeEnum operationType);

        void findLeader();

        void handleLeaderCrash();

        [[noreturn]] void startAsLeader();

        [[noreturn]] void startAsFollower();

        void printNewlyInstalledView();

    public:

        MembershipService(int membershipPort, std::function<std::set<PeerId>(void)> alivePeersGetter_);

        std::set<PeerId> getGroupMembers();

        void handlePeerFailure(PeerId crashedPeerId);

        void start();
    };
}
#endif //LAB2_MEMBERSHIP_H
