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
#include <queue>
#include "network_utils.h"
#include "message.h"

#define MULTICAST_PORT 10001

namespace lab1 {

    typedef std::unordered_map<std::string, std::shared_ptr<UDPSender>> UdpSenderMap;
    //Map of messageId -> (senderId -> proposedSequenceId)
    typedef std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t>> ProposedSeqIdMap;
    typedef std::function<void(const DataMessage)> MsgDeliveryCb;

    template<typename T>
    class ContinuousMsgSender {
        class MsgHolder {
        public:
            T orgMsg;
            const char *serializedMsg;
            std::unordered_set<std::string> recipients;

            MsgHolder(const T &orgMsg, const char *serializedMsg, const std::vector<std::string> &recipients);
        };

        const std::chrono::milliseconds sendingInterval;
        const std::vector<std::string> recipients;
        const std::function<char *(T)> serializer;
        UdpSenderMap udpSenderMap;
        std::mutex msgListMutex;
        std::condition_variable cv;
        bool queueContainsData = false;
        std::vector<MsgHolder> msgList;

    public:

        ContinuousMsgSender(int sendingIntervalMillis,
                            std::vector<std::string> recipients,
                            std::function<char *(T)> serializer);

        void startSendingMessages();

        void queueMsg(T message);

        bool removeRecipient(uint32_t messageId, const std::string &recipient);
    };

    class HoldBackQueue {
        class PendingMsg {
        public:
            DataMessage dataMsg;
            SeqMessage seqMsg;
            bool deliverable;

            explicit PendingMsg(const DataMessage &dataMsg);

            bool operator<(const PendingMsg &other) const;
        };

        class PendingMsgPairHash {
        public:
            std::size_t operator()(const std::pair<uint32_t, uint32_t> &pair) const {
                std::size_t msgIdHash = std::hash<uint32_t>()(pair.first);
                std::size_t senderIdHash = std::hash<uint32_t>()(pair.second);
                return msgIdHash ^ (senderIdHash << 1);
            }
        };

        const MsgDeliveryCb cb;
        // set of pair<msgId, senderId>
        std::unordered_set<std::pair<uint32_t, uint32_t>, PendingMsgPairHash> pendingMsgSet;
        std::deque<PendingMsg> heap;

    public:
        HoldBackQueue(MsgDeliveryCb cb);

        /**
         * Adds DataMessage to the HoldBackQueue
         * @param dataMsg
         * @return true if the message was added, false if the message already existed
         */
        bool addToQueue(DataMessage dataMsg);

        bool markDeliverable(SeqMessage seqMsg);
    };

    class MulticastService {

        const int senderId;
        const std::vector<std::string> recipients;
        const std::chrono::milliseconds messageDelay;
        const double dropRate;

        std::unordered_map<int, std::string> recipientIdMap;

        uint32_t msgId;
        uint32_t latestSeqId;
        ProposedSeqIdMap proposedSeqIdMap;
        HoldBackQueue holdBackQueue;

        ContinuousMsgSender<DataMessage> dataMsgSender;
        ContinuousMsgSender<SeqMessage> seqMsgSender;
        UdpSenderMap udpSenderMap;
        UDPReceiver udpReceiver;

        DataMessage createDataMessage(uint32_t data);

        AckMessage createAckMessage(DataMessage dataMsg);

        static SeqMessage createSeqMessage(AckMessage ackMsg, const std::unordered_map<uint32_t, uint32_t> &proposedSeqIds);

        static SeqAckMessage createSeqAckMessage(SeqMessage param);

        void processDataMsg(DataMessage dataMsg);

        void processAckMsg(AckMessage ackMsg);

        void processSeqMsg(SeqMessage seqMsg);

        void processSeqAckMsg(SeqAckMessage seqAckMsg);

        bool dropMessage(const Message &message, MessageType type) const;

        void delayMessage(MessageType type);

        [[noreturn]] void startListeningForMessages();

    public:
        MulticastService(uint32_t senderId, std::vector<std::string> recipients, const MsgDeliveryCb &cb,
                         double dropRate,
                         int messageDelayMillis);

        void multicast(uint32_t data);

        void start();
    };

}

#endif //LAB1_MULTICAST_H
