//
// Created by sumeet on 10/15/20.
//

#ifndef LAB2_MEMBERSHIP_H
#define LAB2_MEMBERSHIP_H

#include <unordered_map>
#include <vector>
#include <bits/unordered_set.h>
#include <set>

#include "message.h"
#include "network_utils.h"

namespace lab2 {
    typedef std::unordered_map<std::string, PeerId> HostnameToPeerIdMap;
    typedef std::unordered_map<PeerId, std::string> PeerIdToHostnameMap;
    typedef std::unordered_map<PeerId, TcpClient> TcpClientMap;
    typedef std::unordered_map<PeerId, UDPSender> UDPSendersMap;

    class PeerCrashedException : public std::runtime_error {
    public:
        explicit PeerCrashedException(const std::string &message);
    };

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
        std::set<PeerId> groupMembers;
        TcpClientMap tcpClientMap;
        RequestMsg pendingRequest;

        RequestMsg createRequestMsg(PeerId newPeerId);

        OkMsg createOkMsg() const;

        void sendRequestMsg(PeerId peerId, const RequestMsg &requestMsg);

        void sendOkMsg(const OkMsg &okMsg);

        void sendNewViewMsg();

        void waitForAddRequestMsg();

        void waitForNewViewMsg();

        void waitForOkMsg(PeerId peerId, RequestId expectedRequestId);

        bool conflict(PeerId peerId) const;

        void addPeerToMembershipList(PeerId newPeerId);

        static void checkIfPeerCrashed(PeerId peerId, const Message &message);

        [[noreturn]] void startListening();

        [[noreturn]] void connectToLeader();

        void printNewlyInstalledView();

    public:

        MembershipService(int membershipPort, int heartBeatPort,
                          std::vector<std::string> allPeerHostnames_);

        void start();
    };
}
#endif //LAB2_MEMBERSHIP_H
