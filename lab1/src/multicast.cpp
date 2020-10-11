//
// Created by sumeet on 10/2/20.
//

#include <utility>
#include <cmath>

#include <glog/logging.h>

#include "multicast.h"
#include "serde.h"
#include "utils.h"

namespace lab1 {


    template<typename T>
    ContinuousMsgSender<T>::ContinuousMsgSender(int maxSendingIntervalMillis,
                                                const std::vector<std::string> &recipients,
                                                std::function<void(T, char *)> serializer) :
            maxSendingIntervalMillis(maxSendingIntervalMillis),
            recipients(recipients),
            serializer(serializer) {
        retryCount = 0;
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
                    VLOG(1) << "sending " << typeid(T).name() << "-message: " << msgHolder.orgMsg.msg_id
                            << ", recipientSize: " << msgHolder.recipients.size();
                    for (const auto &recipient : msgHolder.recipients) {
                        VLOG(1) << "sending recipient: " << recipient << " message: " << msgHolder.orgMsg;
                        udpSenderMap.at(recipient)->send(msgHolder.serializedMsg, sizeof(T));
                    }
                }
            }
            std::chrono::milliseconds milliseconds{getSendingInterval()};
            LOG(INFO) << typeid(T).name() << " sender, queueSize: " << msgList.size() << ", sleeping for "
                      << milliseconds.count() << "ms";
            std::this_thread::sleep_for(milliseconds);
        }
        LOG(INFO) << "stopping sending data messages";
    }

    template<typename T>
    void ContinuousMsgSender<T>::queueMsg(T message) {
        VLOG(1) << "queueing " << typeid(T).name() << ": " << message;
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
        VLOG(1) << "removing recipient: " << recipient << " for id: " << typeid(T).name() << "-" << messageId;
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

    template<typename T>
    long ContinuousMsgSender<T>::getSendingInterval() {

        long interval = std::pow(2, retryCount++) * 200;
        if (interval > maxSendingIntervalMillis) {
            retryCount = 0;
            interval = 200;
        }
        return interval;
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
            dataMsgSender(4000, recipients, Serde::serializeDataMessage),
            seqMsgSender(4000, recipients, Serde::serializeSeqMessage),
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
        std::ostringstream oss;
        for (const auto &pair : proposedSeqIds) {
            oss << "process " << pair.first << " -> " << pair.second << "\n";
        }
        LOG(INFO) << "\n========================= start of proposed seq ids for msgId: " << ackMsg.msg_id
                  << ", sender:" << ackMsg.sender << " =========================\n"
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

        SeqMessage seqMsg;
        seqMsg.type = MessageType::Seq;
        seqMsg.sender = ackMsg.sender;
        seqMsg.msg_id = ackMsg.msg_id;
        seqMsg.final_seq = finalSeqId;
        seqMsg.final_seq_proposer = finalSeqIdProposer;
        LOG(INFO) << "seqMsg: " << seqMsg;
        return seqMsg;
    }

    SeqAckMessage MulticastService::createSeqAckMessage(SeqMessage seqMsg) const {
        VLOG(1) << "creating SeqAckMsg for seqMsg: " << seqMsg;
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
            LOG(INFO) << "waiting for multicast messages";
            auto message = udpReceiver.receive();
            auto messageType = Serde::getMessageType(message);
            LOG(INFO) << "received " << messageType << " from " << message.sender;
            if (dropMessage(message, messageType)) {
                continue;
            }

            switch (messageType) {
                case MessageType::Data:
                    processDataMsg(Serde::deserializeDataMsg(message));
                    break;
                case MessageType::Ack:
                    processAckMsg(Serde::deserializeAckMessage(message));
                    break;
                case MessageType::Seq:
                    processSeqMsg(Serde::deserializeSeqMessage(message));
                    break;
                case MessageType::SeqAck:
                    processSeqAckMsg(Serde::deserializeSeqAckMessage(message));
                    break;
                default:
                    LOG(ERROR) << "unknown msg type: " << messageType;
            }
        }
        LOG(INFO) << "stopping listening for multicast messages";
    }

    void MulticastService::processDataMsg(DataMessage dataMsg) {
        VLOG(1) << "processing dataMsg: " << dataMsg;
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
        Serde::serializeAckMessage(ackMsg, reinterpret_cast<char *>(&buffer));
        auto sender = recipientIdMap.at(dataMsg.sender);
        delayMessage(MessageType::Ack);
        udpSenderMap.at(sender)->send(buffer, sizeof(AckMessage));
        LOG_IF(WARNING, !added) << "received duplicate dataMsg: " << dataMsg;
    }

    void MulticastService::processAckMsg(AckMessage ackMsg) {
        auto removed = dataMsgSender.removeRecipient(ackMsg.msg_id, recipientIdMap.at(ackMsg.proposer));
        if (removed) {
            VLOG(1) << "processing ackMsg: " << ackMsg;
            if (proposedSeqIdMap.find(ackMsg.msg_id) == proposedSeqIdMap.end()) {
                proposedSeqIdMap[ackMsg.msg_id] = std::unordered_map<uint32_t, uint32_t>(recipients.size());
            }
            proposedSeqIdMap.at(ackMsg.msg_id)[ackMsg.proposer] = ackMsg.proposed_seq;
            auto proposedSeqIds = proposedSeqIdMap.at(ackMsg.msg_id);
            if (proposedSeqIds.size() == recipients.size()) {
                auto seqMsg = createSeqMessage(ackMsg, proposedSeqIds);
                seqMsgSender.queueMsg(seqMsg);
            } else {
                VLOG(1) << recipients.size() - proposedSeqIds.size()
                        << " ackMsgs remaining for msgId: " << ackMsg.msg_id;
            }
        }
        LOG_IF(WARNING, !removed) << "received duplicate ackMsg: " << ackMsg;
    }

    void MulticastService::processSeqMsg(SeqMessage seqMsg) {
        VLOG(1) << "processing seqMsg: " << seqMsg;
        auto marked = holdBackQueue.markDeliverable(seqMsg);
        if (marked) {
            latestSeqId = std::max(latestSeqId, seqMsg.final_seq);
        }
        auto seqAckMsg = createSeqAckMessage(seqMsg);
        char buffer[sizeof(SeqAckMessage)];
        Serde::serializeSeqAckMessage(seqAckMsg, reinterpret_cast<char *>(&buffer));
        auto sender = recipientIdMap.at(seqMsg.sender);
        delayMessage(MessageType::SeqAck);
        udpSenderMap.at(sender)->send(buffer, sizeof(SeqAckMessage));

        MsgIdentifier msgIdentifier(seqMsg.msg_id, seqMsg.sender);
        VLOG(1) << "removing ackMsg from cache for " << msgIdentifier;
        ackMessageCache.erase(msgIdentifier);

        LOG_IF(WARNING, !marked) << "received duplicate seqMsg: " << seqMsg;
    }

    void MulticastService::processSeqAckMsg(SeqAckMessage seqAckMsg) {
        VLOG(1) << "processing seqAckMsg: " << seqAckMsg;
        auto removed = seqMsgSender.removeRecipient(seqAckMsg.msg_id, recipientIdMap.at(seqAckMsg.ack_sender));
        LOG_IF(WARNING, !removed) << "received duplicate seqAckMsg: " << seqAckMsg;
    }

    bool MulticastService::dropMessage(const Message &message, MessageType type) const {
        bool dropMessage = Utils::getRandomNumber(0, 1) < dropRate;
        LOG_IF(WARNING, dropMessage) << "dropping " << type << " from: " << message.sender;
        return dropMessage;
    }

    void MulticastService::delayMessage(MessageType type) {
        // Delay only 50% of the messages
        bool delayMessage = messageDelay.count() != 0 && Utils::getRandomNumber(0, 1) < 0.5;
        LOG_IF(WARNING, delayMessage) << "delaying messageType: " << type << " by " << messageDelay.count() << "ms";
        if (delayMessage) {
            std::this_thread::sleep_for(messageDelay);
        }
    }

    void MulticastService::start() {
        std::thread msgReceiverThread([&]() { startListeningForMessages(); });
        std::thread dataMsgSenderThread([&]() { dataMsgSender.startSendingMessages(); });
        std::thread seqMsgSenderThread([&]() { seqMsgSender.startSendingMessages(); });

        dataMsgSenderThread.join();
        seqMsgSenderThread.join();
        msgReceiverThread.join();
    }

    PendingMsg::PendingMsg(const DataMessage &dataMsg, uint32_t proposedSeq, uint32_t proposer) :
            dataMsg(dataMsg),
            finalSeqId(proposedSeq),
            finalSeqProposer(proposer),
            deliverable(false) {}

    bool PendingMsg::operator<(const PendingMsg &other) const {
        if (finalSeqId == other.finalSeqId) {
            if (deliverable == other.deliverable) {
                //breaking ties by using the smaller proposer
                return finalSeqProposer < other.finalSeqProposer;
            } else {
                return other.deliverable;
            }
        } else {
            return finalSeqId < other.finalSeqId;
        }
    }

    HoldBackQueue::HoldBackQueue(MsgDeliveryCb cb) : cb(std::move(cb)) {}

    bool HoldBackQueue::addToQueue(DataMessage dataMsg, uint32_t proposedSeq, uint32_t proposer) {
        bool added = false;
        MsgIdentifier msgIdentifier(dataMsg.msg_id, dataMsg.sender);
        if (pendingMsgSet.find(msgIdentifier) == pendingMsgSet.end()) {
            pendingMsgSet.insert(msgIdentifier);
            deque.emplace_back(dataMsg, proposedSeq, proposer);
            added = true;
            LOG(INFO) << "adding dataMsg to holdBackQueue: " << dataMsg << ", holdBackQueue size: " << deque.size();
        }
        LOG_IF(WARNING, !added) << "tried adding duplicate dataMsg to holdBackQueue: " << dataMsg;
        return added;
    }

    bool HoldBackQueue::markDeliverable(SeqMessage seqMsg) {
        MsgIdentifier msgIdentifier(seqMsg.msg_id, seqMsg.sender);
        bool marked = false;
        if (pendingMsgSet.find(msgIdentifier) != pendingMsgSet.end()) {
            for (auto &pendingMsg : deque) {
                if (pendingMsg.dataMsg.msg_id == seqMsg.msg_id &&
                    pendingMsg.dataMsg.sender == seqMsg.sender) {
                    pendingMsg.deliverable = true;
                    pendingMsg.finalSeqId = seqMsg.final_seq;
                    pendingMsg.finalSeqProposer = seqMsg.final_seq_proposer;
                    marked = true;
                    LOG(INFO) << "marked " << msgIdentifier << " deliverable";
                    break;
                }
            }
            std::sort(deque.begin(), deque.end());
            LOG(INFO) << "\n" << deque;
            auto it = deque.begin();
            while (it != deque.end() && it->deliverable) {
                DataMessage &dataMsg = it->dataMsg;
                LOG(INFO) << "delivering dataMsg: " << dataMsg
                          << ", finalSeqId: " << it->finalSeqId
                          << ", finalSeqProposer: " << it->finalSeqProposer;
                cb(dataMsg);
                pendingMsgSet.erase(msgIdentifier);
                it = deque.erase(it);
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
          << ", seqId: " << pendingMsg.finalSeqId
          << ", seqIdProposer: " << pendingMsg.finalSeqProposer
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