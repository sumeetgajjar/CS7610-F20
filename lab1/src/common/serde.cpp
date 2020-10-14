//
// Created by sumeet on 10/11/20.
//

#include "serde.h"
#include "utils.h"
#include <glog/logging.h>

namespace lab1 {

    MessageType Serde::getMessageType(const Message &message) {
        CHECK(message.n > 0) << ", found a msg of size 0 bytes, sender: " << message.sender;
        auto *ptr = reinterpret_cast<const uint32_t *>(message.buffer);
        return static_cast<MessageType>(ntohl(*ptr));
    }

    DataMessage Serde::deserializeDataMsg(const Message &message) {
        VLOG(1) << "deserializing data msg from: " << message.sender;
        CHECK(message.n == sizeof(DataMessage)) << ", buffer size does not match DataMessage size";
        auto *ptr = reinterpret_cast<const DataMessage *>(message.buffer);
        DataMessage msg;
        msg.type = ntohl(ptr->type);
        msg.sender = ntohl(ptr->sender);
        msg.msg_id = ntohl(ptr->msg_id);
        msg.data = ntohl(ptr->data);
        return msg;
    }

    AckMessage Serde::deserializeAckMessage(const Message &message) {
        VLOG(1) << "deserializing ack msg from: " << message.sender;
        CHECK(message.n == sizeof(AckMessage)) << ", buffer size does not match AckMessage size";
        auto *ptr = reinterpret_cast<const AckMessage *>(message.buffer);
        AckMessage msg;
        msg.type = ntohl(ptr->type);
        msg.sender = ntohl(ptr->sender);
        msg.msg_id = ntohl(ptr->msg_id);
        msg.proposed_seq = ntohl(ptr->proposed_seq);
        msg.proposer = ntohl(ptr->proposer);
        return msg;
    }

    SeqMessage Serde::deserializeSeqMessage(const Message &message) {
        VLOG(1) << "deserializing seq msg from: " << message.sender;
        CHECK(message.n == sizeof(SeqMessage)) << ", buffer size does not match SeqMessage size";
        auto *ptr = reinterpret_cast<const SeqMessage *>(message.buffer);
        SeqMessage msg;
        msg.type = ntohl(ptr->type);
        msg.sender = ntohl(ptr->sender);
        msg.msg_id = ntohl(ptr->msg_id);
        msg.final_seq = ntohl(ptr->final_seq);
        msg.final_seq_proposer = ntohl(ptr->final_seq_proposer);
        return msg;
    }

    SeqAckMessage Serde::deserializeSeqAckMessage(const Message &message) {
        VLOG(1) << "deserializing seq ack msg from: " << message.sender;
        CHECK(message.n == sizeof(SeqAckMessage)) << ", buffer size does not match SeqAckMessage size";
        auto *ptr = reinterpret_cast<const SeqAckMessage *>(message.buffer);
        SeqAckMessage msg;
        msg.type = ntohl(ptr->type);
        msg.sender = ntohl(ptr->sender);
        msg.msg_id = ntohl(ptr->msg_id);
        msg.ack_sender = ntohl(ptr->ack_sender);
        return msg;
    }

    MarkerMessage Serde::deserializeMarkerMessage(const Message &message) {
        VLOG(1) << "deserializing marker msg from: " << message.sender;
        CHECK(message.n == sizeof(MarkerMessage)) << ", buffer size does not match MarkerMessage size";
        auto *ptr = reinterpret_cast<const MarkerMessage *>(message.buffer);
        MarkerMessage msg;
        msg.type = ntohl(ptr->type);
        msg.sender = ntohl(ptr->sender);
        return msg;
    }

    void Serde::serializeDataMessage(DataMessage dataMsg, char *buffer) {
        VLOG(1) << "serializing data msg, dataMessage: " << dataMsg;
        auto *msg = reinterpret_cast<DataMessage *>(buffer);
        msg->data = htonl(dataMsg.data);
        msg->sender = htonl(dataMsg.sender);
        msg->msg_id = htonl(dataMsg.msg_id);
        msg->type = htonl(dataMsg.type);
    }

    void Serde::serializeAckMessage(AckMessage ackMsg, char *buffer) {
        VLOG(1) << "serializing ack msg, AckMessage: " << ackMsg;
        auto *msg = reinterpret_cast<AckMessage *>(buffer);
        msg->type = htonl(ackMsg.type);
        msg->sender = htonl(ackMsg.sender);
        msg->msg_id = htonl(ackMsg.msg_id);
        msg->proposed_seq = htonl(ackMsg.proposed_seq);
        msg->proposer = htonl(ackMsg.proposer);
    }

    void Serde::serializeSeqMessage(SeqMessage seqMsg, char *buffer) {
        VLOG(1) << "serializing ack msg, SeqMessage: " << seqMsg;
        auto *msg = reinterpret_cast<SeqMessage *>(buffer);
        msg->type = htonl(seqMsg.type);
        msg->sender = htonl(seqMsg.sender);
        msg->msg_id = htonl(seqMsg.msg_id);
        msg->final_seq = htonl(seqMsg.final_seq);
        msg->final_seq_proposer = htonl(seqMsg.final_seq_proposer);
    }

    void Serde::serializeSeqAckMessage(SeqAckMessage seqAckMsg, char *buffer) {
        VLOG(1) << "serializing seq ack msg, seqAckMessage: " << seqAckMsg;
        auto *msg = reinterpret_cast<SeqAckMessage *>(buffer);
        msg->type = htonl(seqAckMsg.type);
        msg->sender = htonl(seqAckMsg.sender);
        msg->msg_id = htonl(seqAckMsg.msg_id);
        msg->ack_sender = htonl(seqAckMsg.ack_sender);
    }

    void Serde::serializeMarkerMessage(MarkerMessage markerMsg, char *buffer) {
        VLOG(1) << "serializing marker msg, markerMessage: " << markerMsg;
        auto *msg = reinterpret_cast<MarkerMessage *>(buffer);
        msg->type = htonl(markerMsg.type);
        msg->sender = htonl(markerMsg.sender);
    }
}

