//
// Created by sumeet on 10/15/20.
//

#ifndef LAB2_SERDE_H
#define LAB2_SERDE_H

#include <glog/logging.h>

#include "message.h"
#include "network_utils.h"

namespace lab2 {
    class SerDe {
    public:
        static MsgTypeEnum getMsgType(const Message &message);

        static void serializeRequestMsg(const RequestMsg &requestMsg, char *buffer);

        static void serializeOkMsg(const OkMsg &okMsg, char *buffer);

        static void serializeNewViewMsg(const NewViewMsg &newViewMsg, char *buffer);

        static void serializeHeartBeatMsg(const HeartBeatMsg &heartBeatMsg, char *buffer);

        static void serializeNewLeaderMsg(const NewLeaderMsg &newLeaderMsg, char *buffer);

        static RequestMsg deserializeRequestMsg(const Message &message);

        static OkMsg deserializeOkMsg(const Message &message);

        static NewViewMsg deserializeNewViewMsg(const Message &message);

        static HeartBeatMsg deserializeHeartBeatMsg(const Message &message);

        static NewLeaderMsg deserializeNewLeaderMsg(const Message &message);
    };
}

#endif //LAB2_SERDE_H
