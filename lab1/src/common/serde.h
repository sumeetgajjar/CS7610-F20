//
// Created by sumeet on 10/11/20.
//

#ifndef LAB1_SERDE_H
#define LAB1_SERDE_H

#include "message.h"
#include "network_utils.h"

namespace lab1 {
    class Serde {
    public:
        static MessageType getMessageType(const Message &message);

        static DataMessage deserializeDataMsg(const Message &message);

        static AckMessage deserializeAckMessage(const Message &message);

        static SeqMessage deserializeSeqMessage(const Message &message);

        static SeqAckMessage deserializeSeqAckMessage(const Message &message);

        static MarkerMessage deserializeMarkerMessage(const Message &message);

        static void serializeDataMessage(DataMessage dataMsg, char *buffer);

        static void serializeAckMessage(AckMessage ackMsg, char *buffer);

        static void serializeSeqMessage(SeqMessage seqMsg, char *buffer);

        static void serializeSeqAckMessage(SeqAckMessage seqAckMsg, char *buffer);

        static void serializeMarkerMessage(MarkerMessage markerMessage, char *buffer);
    };
}

#endif //LAB1_SERDE_H