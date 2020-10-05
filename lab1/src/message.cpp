//
// Created by sumeet on 10/4/20.
//
#include "message.h"

namespace lab1 {

    std::ostream &operator<<(std::ostream &o, const DataMessage &dataMsg) {
        o << "type: " << dataMsg.type
          << ", sender: " << dataMsg.sender
          << ", msg_id: " << dataMsg.msg_id
          << ", data: " << dataMsg.data;
        return o;
    }

    std::ostream &operator<<(std::ostream &o, const AckMessage &ackMsg) {
        o << "type: " << ackMsg.type
          << ", sender: " << ackMsg.sender
          << ", msg_id: " << ackMsg.msg_id
          << ", proposed_seq: " << ackMsg.proposed_seq
          << ", proposer: " << ackMsg.proposer;
        return o;
    }

    std::ostream &operator<<(std::ostream &o, const SeqMessage &seqMsg) {
        o << "type: " << seqMsg.type
          << ", sender: " << seqMsg.sender
          << ", msg_id: " << seqMsg.msg_id
          << ", final_seq: " << seqMsg.final_seq
          << ", final_seq_proposer: " << seqMsg.final_seq_proposer;
        return o;
    }
}