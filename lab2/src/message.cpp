//
// Created by sumeet on 10/15/20.
//
#include "message.h"

namespace lab2 {
    std::ostream &operator<<(std::ostream &o, const MsgTypeEnum &msgTypeEnum) {
        const auto messageTypeStr = [&]() {
            switch (msgTypeEnum) {
                case REQUEST:
                    return "RequestMsg";
                case OK:
                    return "OkMsg";
                case NEW_VIEW:
                    return "NewViewMsg";
                case HEARTBEAT:
                    return "HeartBeatMsg";
                case NEW_LEADER:
                    return "NewLeaderMsg";
                default:
                    throw std::runtime_error("unknown message type: " + std::to_string(msgTypeEnum));
            }
        }();
        o << messageTypeStr;
        return o;
    }

    std::ostream &operator<<(std::ostream &o, const OperationTypeEnum &operationTypeEnum) {
        const auto operationTypeStr = [&]() {
            switch (operationTypeEnum) {
                case NOTHING:
                    return "Nothing";
                case ADD:
                    return "Add";
                case DEL:
                    return "Del";
                case PENDING:
                    return "Pending";
                default:
                    throw std::runtime_error("unknown operation type: " + std::to_string(operationTypeEnum));
            }
        }();
        o << operationTypeStr;
        return o;
    }

    std::ostream &operator<<(std::ostream &o, const RequestMsg &requestMsg) {
        o << "msgType: " << static_cast<MsgTypeEnum>(requestMsg.msgType)
          << ", requestId: " << requestMsg.requestId
          << ", currentViewId: " << requestMsg.currentViewId
          << ", operationType: " << static_cast<OperationTypeEnum>(requestMsg.operationType)
          << ", peerId: " << requestMsg.peerId;
        return o;
    }

    std::ostream &operator<<(std::ostream &o, const OkMsg &okMsg) {
        o << "msgType: " << static_cast<MsgTypeEnum>(okMsg.msgType)
          << ", requestId: " << okMsg.requestId
          << ", currentViewId: " << okMsg.currentViewId;
        return o;
    }

    std::ostream &operator<<(std::ostream &o, const NewViewMsg &newViewMsg) {
        o << "msgType: " << static_cast<MsgTypeEnum>(newViewMsg.msgType)
          << ", newViewId: " << newViewMsg.newViewId
          << ", numberOfMembers: " << newViewMsg.numberOfMembers;
        o << ", members: {";
        for (int i = 0; i < newViewMsg.numberOfMembers; ++i) {
            if (i != 0) {
                o << ", ";
            }
            o << newViewMsg.members[i];
        }
        o << "}";
        return o;
    }

    std::ostream &operator<<(std::ostream &o, const HeartBeatMsg &heartBeatMsg) {
        o << "msgType: " << static_cast<MsgTypeEnum>(heartBeatMsg.msgType)
          << ", peerId: " << heartBeatMsg.peerId;
        return o;
    }

    std::ostream &operator<<(std::ostream &o, const NewLeaderMsg &newLeaderMsg) {
        o << "msgType: " << static_cast<MsgTypeEnum>(newLeaderMsg.msgType)
          << ", requestId: " << newLeaderMsg.requestId
          << ", currentViewId: " << newLeaderMsg.currentViewId
          << ", operationType: " << static_cast<OperationTypeEnum>(newLeaderMsg.operationType);
        return o;
    }
}