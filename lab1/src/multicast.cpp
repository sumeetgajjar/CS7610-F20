//
// Created by sumeet on 10/2/20.
//

#include <glog/logging.h>

#include <utility>
#include "multicast.h"

namespace lab1 {

    MessageType getMessageType(const Message &message) {
        CHECK(message.n > 0) << ", found a msg of size 0 bytes, sender: " << message.sender;
        auto *ptr = reinterpret_cast<const uint32_t *>(message.buffer);
        return static_cast<MessageType>(ntohl(*ptr));
    }

    DataMessage deserializeDataMsg(const Message &message) {
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

    AckMessage deserializeAckMessage(const Message &message) {
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

    SeqMessage deserializeSeqMessage(const Message &message) {
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

    SeqAckMessage deserializeSeqAckMessage(const Message &message) {
        VLOG(1) << "deserializing seq ack msg from: " << message.sender;
        CHECK(message.n == sizeof(SeqAckMessage)) << ", buffer size does not match SeqAckMessage size";
        auto *ptr = reinterpret_cast<const SeqAckMessage *>(message.buffer);
        SeqAckMessage msg;
        msg.type = ntohl(ptr->type);
        msg.sender = ntohl(ptr->sender);
        msg.msg_id = ntohl(ptr->msg_id);
        return msg;
    }


    char *serializeDataMessage(DataMessage dataMsg) {
        VLOG(1) << "serializing data msg, dataMessage: " << dataMsg;
        DataMessage msg;
        msg.data = htonl(dataMsg.data);
        msg.sender = htonl(dataMsg.sender);
        msg.msg_id = htonl(dataMsg.msg_id);
        msg.type = htonl(dataMsg.type);
        char *buf = reinterpret_cast<char *>(&msg);
        return buf;
    }

    char *serializeAckMessage(AckMessage ackMsg) {
        VLOG(1) << "serializing ack msg, AckMessage: " << ackMsg;
        AckMessage msg;
        msg.type = htonl(ackMsg.type);
        msg.sender = htonl(ackMsg.sender);
        msg.msg_id = htonl(ackMsg.msg_id);
        msg.proposed_seq = htonl(ackMsg.proposed_seq);
        msg.proposer = htonl(ackMsg.proposer);
        char *buf = reinterpret_cast<char *>(&ackMsg);
        return buf;
    }

    char *serializeSeqMessage(SeqMessage seqMsg) {
        VLOG(1) << "serializing ack msg, SeqMessage: " << seqMsg;
        SeqMessage msg;
        msg.type = htonl(seqMsg.type);
        msg.sender = htonl(seqMsg.sender);
        msg.msg_id = htonl(seqMsg.msg_id);
        msg.final_seq = htonl(seqMsg.final_seq);
        msg.final_seq_proposer = htonl(seqMsg.final_seq_proposer);
        char *buf = reinterpret_cast<char *>(&msg);
        return buf;
    }

    char *serializeSeqAckMessage(SeqAckMessage seqAckMsg) {
        VLOG(1) << "serializing seq ack msg, seqAckMessage: " << seqAckMsg;
        SeqAckMessage msg;
        msg.type = htonl(seqAckMsg.type);
        msg.sender = htonl(seqAckMsg.sender);
        msg.msg_id = htonl(seqAckMsg.msg_id);
        char *buf = reinterpret_cast<char *>(&msg);
        return buf;
    }

    template<typename T>
    ContinuousMsgSender<T>::ContinuousMsgSender(int sendingIntervalMillis,
                                                std::vector<std::string> recipients,
                                                std::function<char *(T)> serializer) :
            sendingInterval(sendingIntervalMillis),
            recipients(std::move(recipients)),
            serializer(serializer) {}

    template<typename T>
    ContinuousMsgSender<T>::MsgHolder::MsgHolder(const T &orgMsg,
                                                 const char *serializedMsg,
                                                 const std::vector<std::string> &recipients) :
            orgMsg(orgMsg),
            serializedMsg(serializedMsg) {}

    template<typename T>
    void ContinuousMsgSender<T>::startSendingMessages() {
        for (const auto &recipient : recipients) {
            udpSenderMap[recipient] = std::make_shared<UDPSender>(UDPSender(recipient, MULTICAST_PORT));
        }

        LOG(INFO) << "starting sending " << typeid(T).name() << " messages";
        while (true) {
            if (msgList.empty()) {
                LOG(INFO) << "Waiting for new data messages";
                std::unique_lock<std::mutex> uniqueLock(msgListMutex);
                cv.wait(uniqueLock, [&]() { return queueContainsData; });
            }

            {
                std::lock_guard<std::mutex> lockGuard(msgListMutex);
                for (const MsgHolder &msgHolder : msgList) {
                    for (const auto &recipient : msgHolder.recipients) {
                        LOG(INFO) << "sending recipient: " << recipient << " message: " << msgHolder.orgMsg;
                        udpSenderMap.at(recipient)->send(msgHolder.serializedMsg, sizeof(T));
                    }
                }
                LOG(INFO) << typeid(T).name() << " sender sleeping for " << sendingInterval.count() << "ms";
                std::this_thread::sleep_for(sendingInterval);
            }
        }
        LOG(INFO) << "stopping sending data messages";
    }

    template<typename T>
    void ContinuousMsgSender<T>::queueMsg(T message) {
        LOG(INFO) << "queueing " << typeid(T).name() << ": " << message;
        auto serializedMsg = serializer(message);
        {
            std::lock_guard<std::mutex> lockGuard(msgListMutex);
            msgList.emplace_back(message, serializedMsg, recipients);
            queueContainsData = true;
        }
        cv.notify_all();
    }

    template<typename T>
    bool ContinuousMsgSender<T>::removeRecipient(uint32_t messageId, const std::string &recipient) {
        LOG(INFO) << "removing recipient: " << recipient << " for id: " << typeid(T).name() << "-" << messageId;
        bool msgFound = false, removed = false;
        std::lock_guard<std::mutex> lockGuard(msgListMutex);
        for (MsgHolder &msgHolder : msgList) {
            if (messageId == msgHolder.orgMsg.msg_id) {
                msgFound = true;
                removed = msgHolder.recipients.erase(recipient);
                LOG_IF(INFO, removed) << "removed recipient: " << recipient
                                      << ", id: " << typeid(T).name() << "-" << messageId;
                LOG_IF(WARNING, !removed) << "duplicate remove for recipient: " << recipient
                                          << ", id: " << typeid(T).name() << "-" << messageId;
                break;
            }
        }
        auto itr = std::find_if(msgList.begin(), msgList.end(),
                                [](const MsgHolder &msgHolder) { return msgHolder.recipients.size() == 0; });
        if (itr != msgList.end()) {
            LOG(INFO) << "removing " << typeid(T).name() << "-" << itr->orgMsg << ", from msg list";
            msgList.erase(itr);
            LOG(INFO) << typeid(T).name() << " list size:" << msgList.size();
        }

        LOG_IF(WARNING, !msgFound) << "duplicate remove for message id: " << typeid(T).name() << "-" << messageId;
        return removed;
    }

    MulticastService::MulticastService(uint32_t senderId, std::vector<std::string> recipients, const MsgDeliveryCb &cb,
                                       double dropRate,
                                       int messageDelayMillis) :
            senderId(senderId),
            recipients(std::move(recipients)),
            dropRate(dropRate),
            messageDelay(messageDelayMillis),
            holdBackQueue(cb),
            dataMsgSender(4000, recipients, serializeDataMessage),
            seqMsgSender(4000, recipients, serializeSeqMessage),
            udpReceiver(MULTICAST_PORT) {

        msgId = 0;
        latestSeqId = 0;

        for (const auto &recipient : recipients) {
            udpSenderMap[recipient] = std::make_shared<UDPSender>(UDPSender(recipient, MULTICAST_PORT));
        }
    }

    void MulticastService::multicast(const uint32_t data) {
        LOG(INFO) << "multicasting data: " << data;
        auto dataMessage = createDataMessage(data);
        dataMsgSender.queueMsg(dataMessage);
    }

    DataMessage MulticastService::createDataMessage(uint32_t data) {
        VLOG(1) << "creating data msg, data: " << data;
        DataMessage dataMessage;
        dataMessage.data = data;
        dataMessage.sender = senderId;
        dataMessage.type = MessageType::Data;
        dataMessage.msg_id = ++msgId;
        VLOG(1) << "data msg created, dataMsg: " << dataMessage;
        return dataMessage;
    }

    AckMessage MulticastService::createAckMessage(DataMessage dataMsg) {
        VLOG(1) << "creating ack msg for dataMsg: " << dataMsg;
        AckMessage ackMsg;
        ackMsg.type = MessageType::Ack;
        ackMsg.sender = dataMsg.sender;
        ackMsg.msg_id = dataMsg.msg_id;
        ackMsg.proposed_seq = ++latestSeqId;
        ackMsg.proposer = senderId;
        return ackMsg;
    }

    SeqMessage MulticastService::createSeqMessage(AckMessage ackMsg,
                                                  const std::unordered_map<uint32_t, uint32_t> &proposedSeqIds) {
        auto maxElement = std::max_element(proposedSeqIds.begin(), proposedSeqIds.end(),
                                           [](const std::pair<uint32_t, uint32_t> &p1,
                                              const std::pair<uint32_t, uint32_t> &p2) {
                                               if (p1.second == p2.second) {
                                                   //breaking ties by using the smaller proposer
                                                   return p1.first > p2.first;
                                               } else {
                                                   return p1.second < p2.second;
                                               }
                                           });

        uint32_t finalSeqId = maxElement->second;
        uint32_t finalSeqIdProposer = maxElement->first;

        SeqMessage seqMsg;
        seqMsg.type = ackMsg.type;
        seqMsg.sender = ackMsg.sender;
        seqMsg.msg_id = ackMsg.msg_id;
        seqMsg.final_seq = finalSeqId;
        seqMsg.final_seq_proposer = finalSeqIdProposer;
        return seqMsg;
    }

    SeqAckMessage MulticastService::createSeqAckMessage(SeqMessage seqMsg) {
        SeqAckMessage seqAckMsg;
        seqAckMsg.type = MessageType::SeqAck;
        seqAckMsg.sender = seqMsg.sender;
        seqAckMsg.msg_id = seqMsg.msg_id;
        return seqAckMsg;
    }

    [[noreturn]] void MulticastService::startListeningForMessages() {
        LOG(INFO) << "starting listening for multicast messages";
        while (true) {
            auto message = udpReceiver.receive();
            auto messageType = getMessageType(message);
            LOG(INFO) << "received " << messageType << " msg from " << message.sender;
            if (dropMessage(message, messageType)) {
                continue;
            }

            switch (messageType) {
                case MessageType::Data:
                    processDataMsg(deserializeDataMsg(message));
                    break;
                case MessageType::Ack:
                    processAckMsg(deserializeAckMessage(message));
                    break;
                case MessageType::Seq:
                    processSeqMsg(deserializeSeqMessage(message));
                    break;
                case MessageType::SeqAck:
                    processSeqAckMsg(deserializeSeqAckMessage(message));
                    break;
                default:
                    LOG(FATAL) << "unknown msg type: " << messageType;
            }
        }
        LOG(INFO) << "stopping listening for multicast messages";
    }

    void MulticastService::processDataMsg(DataMessage dataMsg) {
        LOG(INFO) << "processing dataMsg: " << dataMsg;
        auto added = holdBackQueue.addToQueue(dataMsg);
        if (added) {
            auto ackMsg = createAckMessage(dataMsg);
            auto buff = serializeAckMessage(ackMsg);
            auto sender = recipientIdMap.at(dataMsg.sender);
            delayMessage(MessageType::Ack);
            udpSenderMap.at(sender)->send(buff, sizeof(AckMessage));
        }
        LOG_IF(WARNING, !added) << "received duplicate dataMsg: " << dataMsg;
    }

    void MulticastService::processAckMsg(AckMessage ackMsg) {
        auto removed = dataMsgSender.removeRecipient(ackMsg.msg_id, recipientIdMap.at(ackMsg.proposer));
        if (removed) {
            LOG(INFO) << "processing ackMsg: " << ackMsg;
            if (proposedSeqIdMap.find(ackMsg.msg_id) == proposedSeqIdMap.end()) {
                proposedSeqIdMap[ackMsg.msg_id] = std::unordered_map<uint32_t, uint32_t>(recipients.size());
            }
            auto proposedSeqIds = proposedSeqIdMap.at(ackMsg.msg_id);
            proposedSeqIds[ackMsg.proposer] = ackMsg.proposed_seq;
            if (proposedSeqIds.size() == recipients.size()) {
                auto seqMsg = createSeqMessage(ackMsg, proposedSeqIds);
                seqMsgSender.queueMsg(seqMsg);
            }
        }
        LOG_IF(WARNING, !removed) << "received duplicate ackMsg: " << ackMsg;
    }

    void MulticastService::processSeqMsg(SeqMessage seqMsg) {
        LOG(INFO) << "processing seqMsg: " << seqMsg;
        auto marked = holdBackQueue.markDeliverable(seqMsg);
        if (marked) {
            latestSeqId = std::max(latestSeqId, seqMsg.final_seq);
            auto seqAckMsg = createSeqAckMessage(seqMsg);
            auto buff = serializeSeqAckMessage(seqAckMsg);
            auto sender = recipientIdMap.at(seqMsg.sender);
            delayMessage(MessageType::SeqAck);
            udpSenderMap.at(sender)->send(buff, sizeof(SeqAckMessage));
        }
        LOG_IF(WARNING, !marked) << "received duplicate seqMsg: " << seqMsg;
    }

    void MulticastService::processSeqAckMsg(SeqAckMessage seqAckMsg) {
        LOG(INFO) << "processing seqAckMsg: " << seqAckMsg;
        auto removed = seqMsgSender.removeRecipient(seqAckMsg.msg_id, recipientIdMap.at(seqAckMsg.sender));
        LOG_IF(WARNING, !removed) << "received duplicate seqAckMsg: " << seqAckMsg;
    }

    bool MulticastService::dropMessage(const Message &message, MessageType type) const {
        //todo implement this
        double random = 10000;
        bool dropMessage = random < dropRate;
        LOG_IF(WARNING, dropMessage) << "dropping " << type << " message from: " << message.sender;
        return dropMessage;
    }

    void MulticastService::delayMessage(MessageType type) {
        //todo implement this
        bool delayMessage = false;
        LOG_IF(WARNING, delayMessage) << "delaying messageType: " << type << " by " << messageDelay.count() << "ms";
    }

    void MulticastService::start() {
        std::thread msgReceiverThread([&]() { startListeningForMessages(); });
        std::thread dataMsgSenderThread([&]() { dataMsgSender.startSendingMessages(); });
        std::thread seqMsgSenderThread([&]() { seqMsgSender.startSendingMessages(); });

        dataMsgSenderThread.join();
        seqMsgSenderThread.join();
        msgReceiverThread.join();
    }

    HoldBackQueue::PendingMsg::PendingMsg(const DataMessage &dataMsg) : dataMsg(dataMsg),
                                                                        deliverable(false) {}

    bool HoldBackQueue::PendingMsg::operator<(const HoldBackQueue::PendingMsg &other) const {
        if (seqMsg.final_seq == other.seqMsg.final_seq) {
            if (deliverable == other.deliverable) {
                //breaking ties by using the smaller proposer
                return seqMsg.final_seq_proposer < other.seqMsg.final_seq_proposer;
            } else {
                return other.deliverable;
            }
        } else {
            return seqMsg.final_seq < other.seqMsg.final_seq;
        }
    }

    HoldBackQueue::HoldBackQueue(MsgDeliveryCb cb) : cb(std::move(cb)) {}

    bool HoldBackQueue::addToQueue(DataMessage dataMsg) {
        bool added = false;
        auto pair = std::make_pair(dataMsg.msg_id, dataMsg.sender);
        if (pendingMsgSet.find(pair) == pendingMsgSet.end()) {
            pendingMsgSet.insert(pair);
            heap.emplace_back(dataMsg);
            added = true;
            LOG(INFO) << "adding dataMsg to holdBackQueue: " << dataMsg << ", holdBackQueue size: " << heap.size();
        }
        LOG_IF(WARNING, !added) << "tried adding duplicate dataMsg to holdBackQueue: " << dataMsg;
        return added;
    }

    bool HoldBackQueue::markDeliverable(SeqMessage seqMsg) {
        auto pair = std::make_pair(seqMsg.msg_id, seqMsg.sender);
        bool marked = false;
        if (pendingMsgSet.find(pair) == pendingMsgSet.end()) {
            for (auto &pendingMsg : heap) {
                if (pendingMsg.dataMsg.msg_id == seqMsg.msg_id &&
                    pendingMsg.dataMsg.sender == seqMsg.sender) {
                    pendingMsg.seqMsg = seqMsg;
                    pendingMsg.deliverable = true;
                    marked = true;
                    break;
                }
            }
            std::sort(heap.begin(), heap.end());
            auto it = heap.begin();
            while (it != heap.end() && it->deliverable) {
                DataMessage &dataMsg = it->dataMsg;
                LOG(INFO) << "delivering dataMsg: " << dataMsg << ", seqMsg: " << it->seqMsg;
                cb(dataMsg);
                pendingMsgSet.erase(std::make_pair(dataMsg.msg_id, dataMsg.sender));
                it = heap.erase(it);
            }

        }
        LOG_IF(WARNING, !marked) << "tried marking seqMsg which no longer exists: " << seqMsg;
        return marked;
    }
}