//
// Created by sumeet on 10/15/20.
//

#include "serde.h"

namespace lab2 {

    MsgTypeEnum SerDe::getMsgType(const Message &message) {
        VLOG(1) << "inside getMsgType, size: " << message.n << ", sender: " << message.sender;
        CHECK(message.n > 0) << ", found a msg of size 0 bytes, sender: " << message.sender;
        auto *ptr = reinterpret_cast<const MsgType *>(message.buffer);
        return static_cast<MsgTypeEnum>(::ntohl(*ptr));
    }

    void SerDe::serializeRequestMsg(const RequestMsg &requestMsg, char *buffer) {
        VLOG(1) << "serializing RequestMsg: " << requestMsg;
        auto *ptr = reinterpret_cast<RequestMsg *>(buffer);
        ptr->msgType = ::htonl(requestMsg.msgType);
        ptr->requestId = ::htonl(requestMsg.requestId);
        ptr->currentViewId = ::htonl(requestMsg.currentViewId);
        ptr->operationType = ::htonl(requestMsg.operationType);
        ptr->peerId = ::htonl(requestMsg.peerId);
    }

    void SerDe::serializeOkMsg(const OkMsg &okMsg, char *buffer) {
        VLOG(1) << "serializing OkMsg: " << okMsg;
        auto *ptr = reinterpret_cast<OkMsg *>(buffer);
        ptr->msgType = ::htonl(okMsg.msgType);
        ptr->requestId = ::htonl(okMsg.requestId);
        ptr->currentViewId = ::htonl(okMsg.currentViewId);
    }

    void SerDe::serializeNewViewMsg(const NewViewMsg &newViewMsg, char *buffer) {
        VLOG(1) << "serializing NewViewMsg: " << newViewMsg;
        auto *ptr = reinterpret_cast<NewViewMsg *>(buffer);
        ptr->msgType = ::htonl(newViewMsg.msgType);
        ptr->newViewId = ::htonl(newViewMsg.newViewId);
        ptr->numberOfMembers = ::htonl(newViewMsg.numberOfMembers);
        for (int i = 0; i < newViewMsg.numberOfMembers; ++i) {
            ptr->members[i] = ::htonl(newViewMsg.members[i]);
        }
    }

    void SerDe::serializeHeartBeatMsg(const HeartBeatMsg &heartBeatMsg, char *buffer) {
        VLOG(1) << "serializing HeartBeatMsg: " << heartBeatMsg;
        auto *ptr = reinterpret_cast<HeartBeatMsg *>(buffer);
        ptr->msgType = ::htonl(heartBeatMsg.msgType);
        ptr->peerId = ::htonl(heartBeatMsg.peerId);
    }

    void SerDe::serializeNewLeaderMsg(const NewLeaderMsg &newLeaderMsg, char *buffer) {
        VLOG(1) << "serializing NewLeaderMsg: " << newLeaderMsg;
        auto *ptr = reinterpret_cast<NewLeaderMsg *>(buffer);
        ptr->msgType = ::htonl(newLeaderMsg.msgType);
        ptr->requestId = ::htonl(newLeaderMsg.requestId);
        ptr->currentViewId = ::htonl(newLeaderMsg.currentViewId);
        ptr->operationType = ::htonl(newLeaderMsg.operationType);
    }

    RequestMsg SerDe::deserializeRequestMsg(const Message &message) {
        VLOG(1) << "deserializing RequestMsg from sender: " << message.sender;
        CHECK(sizeof(RequestMsg) == message.n) << ", buffer size does not match RequestMsg size: " << message.n;
        const auto *ptr = reinterpret_cast<const RequestMsg *>(message.buffer);
        RequestMsg msg;
        msg.msgType = ::ntohl(ptr->msgType);
        msg.requestId = ::ntohl(ptr->requestId);
        msg.currentViewId = ::ntohl(ptr->currentViewId);
        msg.operationType = ::ntohl(ptr->operationType);
        msg.peerId = ::ntohl(ptr->peerId);
        return msg;
    }

    OkMsg SerDe::deserializeOkMsg(const Message &message) {
        VLOG(1) << "deserializing OkMsg from sender: " << message.sender;
        CHECK(sizeof(OkMsg) == message.n) << ", buffer size does not match OkMsg size: " << message.n;
        auto *ptr = reinterpret_cast<const OkMsg *>(message.buffer);
        OkMsg msg;
        msg.msgType = ::ntohl(ptr->msgType);
        msg.requestId = ::ntohl(ptr->requestId);
        msg.currentViewId = ::ntohl(ptr->currentViewId);
        return msg;
    }

    NewViewMsg SerDe::deserializeNewViewMsg(const Message &message) {
        VLOG(1) << "deserializing NewViewMsg from sender: " << message.sender;
        CHECK(sizeof(NewViewMsg) == message.n) << ", buffer size does not match NewViewMsg size: " << message.n;
        auto *ptr = reinterpret_cast<const NewViewMsg *>(message.buffer);
        NewViewMsg msg;
        msg.msgType = ::ntohl(ptr->msgType);
        msg.newViewId = ::ntohl(ptr->newViewId);
        msg.numberOfMembers = ::ntohl(ptr->numberOfMembers);
        for (int i = 0; i < msg.numberOfMembers; ++i) {
            msg.members[i] = ::ntohl(ptr->members[i]);
        }
        return msg;
    }

    HeartBeatMsg SerDe::deserializeHeartBeatMsg(const Message &message) {
        VLOG(1) << "deserializing HeartBeatMsg from sender: " << message.sender;
        CHECK(sizeof(HeartBeatMsg) == message.n) << ", buffer size does not match HeartBeatMsg size: " << message.n;
        auto *ptr = reinterpret_cast<const HeartBeatMsg *>(message.buffer);
        HeartBeatMsg msg;
        msg.msgType = ::ntohl(ptr->msgType);
        msg.peerId = ::ntohl(ptr->peerId);
        return msg;
    }

    NewLeaderMsg SerDe::deserializeNewLeaderMsg(const Message &message) {
        VLOG(1) << "deserializing NewLeaderMsg from sender: " << message.sender;
        CHECK(sizeof(NewLeaderMsg) == message.n) << ", buffer size does not match NewLeaderMsg size: " << message.n;
        auto *ptr = reinterpret_cast<const NewLeaderMsg *>(message.buffer);
        NewLeaderMsg msg;
        msg.msgType = ::ntohl(ptr->msgType);
        msg.requestId = ::ntohl(ptr->requestId);
        msg.currentViewId = ::ntohl(ptr->currentViewId);
        msg.operationType = ::ntohl(ptr->operationType);
        return msg;
    }
}