//
// Created by sumeet on 10/2/20.
//

#ifndef LAB1_MULTICAST_H
#define LAB1_MULTICAST_H

#include <utility>
#include <vector>
#include <map>
#include <thread>
#include <functional>
#include <condition_variable>
#include "network_utils.h"
#include "message.h"

#define MULTICAST_PORT 10001

namespace lab1 {

    typedef std::unordered_map<std::string, std::shared_ptr<UDPSender>> UdpSenderMap;
    typedef std::function<void(const DataMessage)> DataMsgDeliveryCb;

    template<typename T>
    class ContinuousMsgSender {
        class MsgHolder {
        public:
            const T orgMsg;
            const char *serializedMsg;
            const std::unordered_set<std::string> recipients;

            MsgHolder(const T &orgMsg, const char *serializedMsg, const std::vector<std::string> &recipients);
        };

        const std::chrono::milliseconds sendingInterval;
        const std::vector<std::string> recipients;
        const std::function<char *(T)> serializer;
        UdpSenderMap udpSenders;
        std::mutex queueMutex;
        std::condition_variable cv;
        bool queueContainsData = false;
        std::vector<MsgHolder> queue;

    public:

        ContinuousMsgSender(std::chrono::milliseconds sendingInterval,
                            std::vector<std::string> recipients,
                            std::function<char *(T)> serializer);

        void startSendingMessages();

        void queueMsg(T message);

        bool removeRecipient(uint32_t messageId, const std::string &recipient);
    };

    class MulticastService {

        const int senderId;
        const std::vector<std::string> recipients;
        const std::function<void(DataMessage)> cb;

        int msgId;
        std::mutex msgIdMutex;

        int seqId;
        std::mutex seqIdMutex;

        ContinuousMsgSender<DataMessage> dataMsgSender;
        UDPReceiver udpReceiver;

        DataMessage createDataMessage(uint32_t data);

    public:
        MulticastService(uint32_t senderId, std::vector<std::string> recipients, std::function<void(DataMessage)> cb);

        void multicast(uint32_t data);

        [[noreturn]] void startListeningForMessages();
    };

}

#endif //LAB1_MULTICAST_H
