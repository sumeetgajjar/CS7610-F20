//
// Created by sumeet on 10/2/20.
//

#include <glog/logging.h>

#include <utility>
#include "multicast.h"
#include "utils.h"

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
        msg.ack_sender = ntohl(ptr->ack_sender);
        return msg;
    }


    void serializeDataMessage(DataMessage dataMsg, char *buffer) {
        VLOG(1) << "serializing data msg, dataMessage: " << dataMsg;
        auto *msg = reinterpret_cast<DataMessage *>(buffer);
        msg->data = htonl(dataMsg.data);
        msg->sender = htonl(dataMsg.sender);
        msg->msg_id = htonl(dataMsg.msg_id);
        msg->type = htonl(dataMsg.type);
    }

    void serializeAckMessage(AckMessage ackMsg, char *buffer) {
        VLOG(1) << "serializing ack msg, AckMessage: " << ackMsg;
        auto *msg = reinterpret_cast<AckMessage *>(buffer);
        msg->type = htonl(ackMsg.type);
        msg->sender = htonl(ackMsg.sender);
        msg->msg_id = htonl(ackMsg.msg_id);
        msg->proposed_seq = htonl(ackMsg.proposed_seq);
        msg->proposer = htonl(ackMsg.proposer);
    }

    void serializeSeqMessage(SeqMessage seqMsg, char *buffer) {
        VLOG(1) << "serializing ack msg, SeqMessage: " << seqMsg;
        auto *msg = reinterpret_cast<SeqMessage *>(buffer);
        msg->type = htonl(seqMsg.type);
        msg->sender = htonl(seqMsg.sender);
        msg->msg_id = htonl(seqMsg.msg_id);
        msg->final_seq = htonl(seqMsg.final_seq);
        msg->final_seq_proposer = htonl(seqMsg.final_seq_proposer);
    }

    void serializeSeqAckMessage(SeqAckMessage seqAckMsg, char *buffer) {
        VLOG(1) << "serializing seq ack msg, seqAckMessage: " << seqAckMsg;
        auto *msg = reinterpret_cast<SeqAckMessage *>(buffer);
        msg->type = htonl(seqAckMsg.type);
        msg->sender = htonl(seqAckMsg.sender);
        msg->msg_id = htonl(seqAckMsg.msg_id);
        msg->ack_sender = htonl(seqAckMsg.ack_sender);
    }

    template<typename T>
    ContinuousMsgSender<T>::ContinuousMsgSender(int sendingIntervalMillis,
                                                const std::vector<std::string> &recipients,
                                                std::function<void(T, char *)> serializer) :
            sendingInterval(sendingIntervalMillis),
            recipients(recipients),
            serializer(serializer) {
        for (const auto &recipient : recipients) {
            udpSenderMap[recipient] = std::make_shared<UDPSender>(UDPSender(recipient, MULTICAST_PORT));
        }
    }

    template<typename T>
    ContinuousMsgSender<T>::MsgHolder::MsgHolder(const T &orgMsg,
                                                 const std::function<void(T, char *)> &serializer,
                                                 const std::vector<std::string> &recipients) :
            orgMsg(orgMsg),
            recipients(recipients.begin(), recipients.end()) {
        serializer(orgMsg, reinterpret_cast<char *>(&serializedMsg));
    }

    template<typename T>
    [[noreturn]] void ContinuousMsgSender<T>::startSendingMessages() {
        LOG(INFO) << "starting sending " << typeid(T).name() << " messages";
        while (true) {
            if (msgList.empty()) {
                LOG(INFO) << "Waiting for new " << typeid(T).name() << " messages";
                std::unique_lock<std::mutex> uniqueLock(msgListMutex);
                cv.wait(uniqueLock, [&]() { return queueContainsData; });
                queueContainsData = false;
            }

            {
                std::lock_guard<std::mutex> lockGuard(msgListMutex);
                LOG(INFO) << "sending " << typeid(T).name() << "-messages, queueSize: " << msgList.size();
                for (const MsgHolder &msgHolder : msgList) {
                    LOG(INFO) << "sending " << typeid(T).name() << "-message: " << msgHolder.orgMsg.msg_id
                              << ", recipientSize: " << msgHolder.recipients.size();
                    for (const auto &recipient : msgHolder.recipients) {
                        LOG(INFO) << "sending recipient: " << recipient << " message: " << msgHolder.orgMsg;
                        udpSenderMap.at(recipient)->send(msgHolder.serializedMsg, sizeof(T));
                    }
                }
                LOG(INFO) << typeid(T).name() << " sender, queueSize: " << msgList.size() << ", sleeping for "
                          << sendingInterval.count() << "ms";
            }
            std::this_thread::sleep_for(sendingInterval);
        }
        LOG(INFO) << "stopping sending data messages";
    }

    template<typename T>
    void ContinuousMsgSender<T>::queueMsg(T message) {
        LOG(INFO) << "queueing " << typeid(T).name() << ": " << message;
        {
            std::lock_guard<std::mutex> lockGuard(msgListMutex);
            msgList.emplace_back(message, serializer, recipients);
            queueContainsData = true;
        }
        LOG(INFO) << "queued: " << message << ", " << typeid(T).name() << "-queueSize: " << msgList.size();
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

    MulticastService::MulticastService(uint32_t senderId, const std::vector<std::string> &recipients,
                                       const MsgDeliveryCb &cb,
                                       double dropRate,
                                       int messageDelayMillis) :
            senderId(senderId),
            recipients(recipients),
            dropRate(dropRate),
            messageDelay(messageDelayMillis),
            holdBackQueue(cb),
            dataMsgSender(4000, recipients, serializeDataMessage),
            seqMsgSender(4000, recipients, serializeSeqMessage),
            udpReceiver(MULTICAST_PORT) {

        msgId = 0;
        latestSeqId = 0;
        LOG(INFO) << "multicast recipientSize: " << this->recipients.size();
        for (const auto &recipient : this->recipients) {
            udpSenderMap[recipient] = std::make_shared<UDPSender>(UDPSender(recipient, MULTICAST_PORT));

            const auto recipientId = Utils::getProcessIdentifier(recipients, recipient);
            recipientIdMap[recipientId] = recipient;
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

    AckMessage MulticastService::createOrGetAckMessage(DataMessage dataMsg, uint32_t proposedSeq, bool createNew) {
        MsgIdentifier msgIdentifier(dataMsg.msg_id, dataMsg.sender);
        if (createNew) {
            VLOG(1) << "creating new ackMsg for dataMsg: " << dataMsg;
            AckMessage ackMsg;
            ackMsg.type = MessageType::Ack;
            ackMsg.sender = dataMsg.sender;
            ackMsg.msg_id = dataMsg.msg_id;
            ackMsg.proposed_seq = proposedSeq;
            ackMsg.proposer = senderId;
            ackMessageCache[msgIdentifier] = ackMsg;
        }
        VLOG_IF(1, !createNew) << "using cached ack msg for dataMsg: " << dataMsg;
        return ackMessageCache.at(msgIdentifier);
    }

    SeqMessage MulticastService::createSeqMessage(AckMessage ackMsg,
                                                  const std::unordered_map<uint32_t, uint32_t> &proposedSeqIds) {
        VLOG(1) << "creating seqMsg for ackMsg: " << ackMsg;
        std::ostringstream oss;
        for (const auto &pair : proposedSeqIds) {
            oss << "process " << pair.first << " -> " << pair.second << "\n";
        }
        VLOG(1) << "\n========================= start of proposed seq ids =========================\n"
                << oss.str()
                << "========================== end of proposed seq ids ==========================";

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
        VLOG(1) << "msgId: " << ackMsg.msg_id << ", sender:" << ackMsg.sender
                << ", finalSeq: " << finalSeqId << ", proposer: " << finalSeqIdProposer;

        SeqMessage seqMsg;
        seqMsg.type = MessageType::Seq;
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
        seqAckMsg.ack_sender = senderId;
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
        uint32_t proposedSeq = latestSeqId + 1;
        auto added = holdBackQueue.addToQueue(dataMsg, proposedSeq, senderId);
        auto ackMsg = createOrGetAckMessage(dataMsg, proposedSeq, added);
        if (added) {
            // If dataMsg is not added to the holdBackQueue then we don't need to increment the seqId since
            // this dataMsg is not a new dataMsg, it is a result of retransmission. The retransmission occurs
            // if ackMsg is not received within certain amount of time.
            latestSeqId++;
        }

        char buffer[sizeof(AckMessage)];
        serializeAckMessage(ackMsg, reinterpret_cast<char *>(&buffer));
        auto sender = recipientIdMap.at(dataMsg.sender);
        delayMessage(MessageType::Ack);
        udpSenderMap.at(sender)->send(buffer, sizeof(AckMessage));
        LOG_IF(WARNING, !added) << "received duplicate dataMsg: " << dataMsg;
    }

    void MulticastService::processAckMsg(AckMessage ackMsg) {
        auto removed = dataMsgSender.removeRecipient(ackMsg.msg_id, recipientIdMap.at(ackMsg.proposer));
        if (removed) {
            LOG(INFO) << "processing ackMsg: " << ackMsg;
            if (proposedSeqIdMap.find(ackMsg.msg_id) == proposedSeqIdMap.end()) {
                proposedSeqIdMap[ackMsg.msg_id] = std::unordered_map<uint32_t, uint32_t>(recipients.size());
            }
            proposedSeqIdMap.at(ackMsg.msg_id)[ackMsg.proposer] = ackMsg.proposed_seq;
            auto proposedSeqIds = proposedSeqIdMap.at(ackMsg.msg_id);
            if (proposedSeqIds.size() == recipients.size()) {
                auto seqMsg = createSeqMessage(ackMsg, proposedSeqIds);
                seqMsgSender.queueMsg(seqMsg);
            } else {
                LOG(INFO) << recipients.size() - proposedSeqIds.size()
                          << " ackMsgs remaining for msgId: " << ackMsg.msg_id;
            }
        }
        LOG_IF(WARNING, !removed) << "received duplicate ackMsg: " << ackMsg;
    }

    void MulticastService::processSeqMsg(SeqMessage seqMsg) {
        LOG(INFO) << "processing seqMsg: " << seqMsg;
        auto marked = holdBackQueue.markDeliverable(seqMsg);
        if (marked) {
            latestSeqId = std::max(latestSeqId, seqMsg.final_seq);
        }
        auto seqAckMsg = createSeqAckMessage(seqMsg);
        char buffer[sizeof(SeqAckMessage)];
        serializeSeqAckMessage(seqAckMsg, reinterpret_cast<char *>(&buffer));
        auto sender = recipientIdMap.at(seqMsg.sender);
        delayMessage(MessageType::SeqAck);
        udpSenderMap.at(sender)->send(buffer, sizeof(SeqAckMessage));

        MsgIdentifier msgIdentifier(seqMsg.msg_id, seqMsg.sender);
        LOG(INFO) << "removing ackMsg from cache for " << msgIdentifier;
        ackMessageCache.erase(msgIdentifier);

        LOG_IF(WARNING, !marked) << "received duplicate seqMsg: " << seqMsg;
    }

    void MulticastService::processSeqAckMsg(SeqAckMessage seqAckMsg) {
        LOG(INFO) << "processing seqAckMsg: " << seqAckMsg;
        auto removed = seqMsgSender.removeRecipient(seqAckMsg.msg_id, recipientIdMap.at(seqAckMsg.ack_sender));
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

    PendingMsg::PendingMsg(const DataMessage &dataMsg, uint32_t proposedSeq, uint32_t proposer) : dataMsg(dataMsg),
                                                                                                  deliverable(false) {
        seqMsg.final_seq = proposedSeq;
        seqMsg.final_seq_proposer = proposer;
    }

    bool PendingMsg::operator<(const PendingMsg &other) const {
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

    bool HoldBackQueue::addToQueue(DataMessage dataMsg, uint32_t proposedSeq, uint32_t proposer) {
        bool added = false;
        MsgIdentifier msgIdentifier(dataMsg.msg_id, dataMsg.sender);
        if (pendingMsgSet.find(msgIdentifier) == pendingMsgSet.end()) {
            pendingMsgSet.insert(msgIdentifier);
            heap.emplace_back(dataMsg, proposedSeq, proposer);
            added = true;
            LOG(INFO) << "adding dataMsg to holdBackQueue: " << dataMsg << ", holdBackQueue size: " << heap.size();
        }
        LOG_IF(WARNING, !added) << "tried adding duplicate dataMsg to holdBackQueue: " << dataMsg;
        return added;
    }

    bool HoldBackQueue::markDeliverable(SeqMessage seqMsg) {
        MsgIdentifier msgIdentifier(seqMsg.msg_id, seqMsg.sender);
        bool marked = false;
        if (pendingMsgSet.find(msgIdentifier) != pendingMsgSet.end()) {
            for (auto &pendingMsg : heap) {
                if (pendingMsg.dataMsg.msg_id == seqMsg.msg_id &&
                    pendingMsg.dataMsg.sender == seqMsg.sender) {
                    pendingMsg.seqMsg = seqMsg;
                    pendingMsg.deliverable = true;
                    marked = true;
                    LOG(INFO) << "marked " << msgIdentifier << " deliverable";
                    break;
                }
            }
            std::sort(heap.begin(), heap.end());
            LOG(INFO) << "\n" << heap;
            auto it = heap.begin();
            while (it != heap.end() && it->deliverable) {
                DataMessage &dataMsg = it->dataMsg;
                LOG(INFO) << "delivering dataMsg: " << dataMsg << ", seqMsg: " << it->seqMsg;
                cb(dataMsg);
                pendingMsgSet.erase(msgIdentifier);
                it = heap.erase(it);
            }

        }
        LOG_IF(WARNING, !marked) << "tried marking seqMsg which no longer exists: " << seqMsg;
        return marked;
    }

    MsgIdentifier::MsgIdentifier(uint32_t msgId, uint32_t sender) : msgId(msgId), sender(sender) {}

    bool MsgIdentifier::operator==(const MsgIdentifier &other) const {
        return msgId == other.msgId && sender == other.sender;
    }

    std::ostream &operator<<(std::ostream &o, const MsgIdentifier &msgIdentifier) {
        o << "msgId: " << msgIdentifier.msgId
          << ", sender: " << msgIdentifier.sender;
        return o;
    }

    std::ostream &operator<<(std::ostream &o, const PendingMsg &pendingMsg) {
        o << "msgId: " << pendingMsg.dataMsg.msg_id
          << ", sender: " << pendingMsg.dataMsg.sender
          << ", seqId: " << pendingMsg.seqMsg.final_seq
          << ", seqIdProposer: " << pendingMsg.seqMsg.final_seq_proposer
          << ", deliverable: " << pendingMsg.deliverable;
        return o;
    }

    std::ostream &operator<<(std::ostream &o, const std::deque<PendingMsg> &deque) {
        o << "============================= start of HoldBackQueue =============================\n";
        for (const auto &pendingMsg : deque) {
            o << pendingMsg << "\n";
        }
        o << "============================== end of HoldBackQueue ==============================\n";
        return o;
    }
}