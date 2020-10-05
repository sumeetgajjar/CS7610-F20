//
// Created by sumeet on 10/2/20.
//

#include <glog/logging.h>

#include <utility>
#include "multicast.h"

namespace lab1 {

    MessageType getMessageType(const Message &message) {
        return MessageType::Data;
    }

    char *serializeDataMessage(DataMessage dataMessage) {
        DataMessage serializedDataMsg;
        serializedDataMsg.data = htonl(dataMessage.data);
        serializedDataMsg.sender = htonl(dataMessage.sender);
        serializedDataMsg.msg_id = htonl(dataMessage.msg_id);
        serializedDataMsg.type = htonl(dataMessage.type);
        char *buf = reinterpret_cast<char *>(&serializedDataMsg);
        return buf;
    }

    template<typename T>
    ContinuousMsgSender<T>::ContinuousMsgSender(std::chrono::milliseconds sendingInterval,
                                                std::vector<std::string> recipients,
                                                std::function<char *(T)> serializer) :
            sendingInterval(sendingInterval),
            recipients(std::move(recipients)),
            serializer(serializer) {}

    template<typename T>
    ContinuousMsgSender<T>::MsgHolder::MsgHolder(const T &orgMsg,
                                                 const char *serializedMsg,
                                                 const std::vector<std::string> &recipients) :
            orgMsg(orgMsg),
            serializedMsg(serializedMsg),
            recipients(recipients.begin(), recipients.end()) {
    }

    template<typename T>
    void ContinuousMsgSender<T>::startSendingMessages() {
        for (const auto &recipient : recipients) {
            udpSenders[recipient] = std::make_shared<UDPSender>(UDPSender(recipient, MULTICAST_PORT));
        }

        LOG(INFO) << "starting sending data messages";
        while (true) {
            if (queue.empty()) {
                LOG(INFO) << "Waiting for new data messages";
                std::unique_lock<std::mutex> uniqueLock(queueMutex);
                cv.wait(uniqueLock, [&]() { return queueContainsData; });
            }

            {
                std::lock_guard<std::mutex> lockGuard(queueMutex);
                for (const MsgHolder &msgHolder : queue) {
                    for (const auto &recipient : msgHolder.recipients) {
                        LOG(INFO) << "sending recipient: " << recipient << " message: " << msgHolder.orgMsg;
                        udpSenders[recipient]->send(msgHolder.serializedMsg, sizeof(T));
                    }
                }
                std::this_thread::sleep_for(sendingInterval);
            }
        }
        LOG(INFO) << "stopping sending data messages";
    }

    template<typename T>
    void ContinuousMsgSender<T>::queueMsg(T message) {
        auto serializedMsg = serializer(message);
        {
            std::lock_guard<std::mutex> lockGuard(queueMutex);
            queue.emplace_back(message, serializedMsg, recipients);
            queueContainsData = true;
        }
        cv.notify_all();
    }

    template<typename T>
    bool ContinuousMsgSender<T>::removeRecipient(uint32_t messageId, const std::string &recipient) {
        std::lock_guard<std::mutex> lockGuard(queueMutex);
        for (const MsgHolder &msgHolder : queue) {
            if (messageId == msgHolder.orgMsg.msg_id) {
                return msgHolder.recipients.erase(recipient);
            }
        }
        return false;
    }

    MulticastService::MulticastService(uint32_t senderId,
                                       std::vector<std::string> recipients,
                                       DataMsgDeliveryCb cb) :
            senderId(senderId),
            recipients(std::move(recipients)),
            cb(std::move(cb)),
            dataMsgSender(std::chrono::milliseconds{4000}, recipients, serializeDataMessage),
            udpReceiver(MULTICAST_PORT) {
        msgId = 0;
        seqId = 0;
        dataMsgSender.startSendingMessages();
    }

    void MulticastService::multicast(const uint32_t data) {
        auto dataMessage = createDataMessage(data);
        dataMsgSender.queueMsg(dataMessage);
    }

    DataMessage MulticastService::createDataMessage(uint32_t data) {
        std::lock_guard<std::mutex> lockGuard(msgIdMutex);
        DataMessage dataMessage;
        dataMessage.data = data;
        dataMessage.sender = senderId;
        dataMessage.type = MessageType::Data;
        dataMessage.msg_id = ++msgId;
        return dataMessage;
    }

    [[noreturn]] void MulticastService::startListeningForMessages() {
        LOG(INFO) << "starting listening for multicast messages";
        while (true) {
            auto message = udpReceiver.receive();

        }
        LOG(INFO) << "stopping listening for multicast messages";
    }
}