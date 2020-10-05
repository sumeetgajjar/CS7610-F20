//
// Created by sumeet on 9/30/20.
//

#ifndef LAB1_MESSAGE_H
#define LAB1_MESSAGE_H

#include <cstdint>
#include <ostream>

namespace lab1 {

    enum MessageType : uint32_t {
        Data = 1,
        Ack = 2,
        Seq = 3
    };

    typedef struct {
        uint32_t type; // must be equal to 1
        uint32_t sender; // the sender’s id
        uint32_t msg_id; // the identifier of the message generated by the sender
        uint32_t data; // a dummy integer
    } DataMessage;

    typedef struct {
        uint32_t type; // must be equal to 2
        uint32_t sender; // the sender of the DataMessage
        uint32_t msg_id; // the identifier of the DataMessage generated by the sender
        uint32_t proposed_seq; // the proposed sequence number
        uint32_t proposer; // the process id of the proposer
    } AckMessage;

    typedef struct {
        uint32_t type; // must be equal to 3
        uint32_t sender; // the sender of the DataMessage
        uint32_t msg_id; // the identifier of the DataMessage generated by the sender
        uint32_t final_seq; // the final sequence number selected by the sender
        uint32_t final_seq_proposer; // the process id of the proposer who proposed the final_seq
    } SeqMessage;

    std::ostream &operator<<(std::ostream &o, const DataMessage &dataMsg);

    std::ostream &operator<<(std::ostream &o, const AckMessage &ackMsg);

    std::ostream &operator<<(std::ostream &o, const SeqMessage &seqMsg);
}
#endif //LAB1_MESSAGE_H
