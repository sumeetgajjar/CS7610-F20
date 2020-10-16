//
// Created by sumeet on 10/15/20.
//

#ifndef LAB2_MESSAGE_H
#define LAB2_MESSAGE_H

#include <array>
#include <cstdint>
#include <ostream>

#define MAX_PEERS 10

namespace lab2 {
    typedef uint32_t MsgType;
    typedef uint32_t RequestId;
    typedef uint32_t ViewId;
    typedef uint32_t PeerId;

    enum MessageType {
        REQUEST = 1,
        OK = 2,
        NEW_VIEW = 3,
        HEARTBEAT = 4,
        NEW_LEADER = 5
    };

    enum OperationType {
        NOTHING = 0,
        ADD = 1,
        DEL = 2,
        PENDING = 3
    };

    typedef struct {
        MsgType msgType; // should always be equal to 1
        RequestId requestId;
        ViewId currentViewId;
        OperationType operationType;
        PeerId peerId; //
    } RequestMsg;

    typedef struct {
        MsgType msgType; // should always be equal to 2
        RequestId requestId;
        ViewId currentViewId;
    } OkMsg;

    typedef struct {
        MsgType msgType; // should always be equal to 3
        ViewId newViewId;
        int numberOfMembers;
        std::array<PeerId, MAX_PEERS> members;
    } NewViewMsg;

    typedef struct {
        MsgType msgType; // should always be equal to 4
        PeerId peerId;
    } HeartBeatMsg;

    typedef struct {
        MsgType msgType; // should always be equal to 5
        RequestId requestId;
        ViewId currentViewId;
        OperationType operationType; // should always be equal to 3
    } NewLeaderMsg;

    std::ostream &operator<<(std::ostream &o, const MessageType &messageType);

    std::ostream &operator<<(std::ostream &o, const OperationType &operationType);

    std::ostream &operator<<(std::ostream &o, const RequestMsg &requestMsg);

    std::ostream &operator<<(std::ostream &o, const OkMsg &okMsg);

    std::ostream &operator<<(std::ostream &o, const NewViewMsg &newViewMsg);

    std::ostream &operator<<(std::ostream &o, const HeartBeatMsg &heartBeatMsg);

    std::ostream &operator<<(std::ostream &o, const NewLeaderMsg &newLeaderMsg);
}

#endif //LAB2_MESSAGE_H
