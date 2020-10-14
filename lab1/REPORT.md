# Total Order Mutlicast using ISIS Algorithm

In order to send a multicast message, `MulticastService::multicast(uint32_t data)` is invoked. This call is
non-blocking, `MulticastService` takes the `data` and returns the control to the sender. The task of sending out various
messages is carried out in different threads.

While constructing `MulticastService`, a `MsgDeliveryCb` callback is passed as argument. This callback is invoked when
a multicast message needs to be delivered to the process.

### Implementing Reliable Delivery of Messages using UDP

##### ContinuousMsgSender

Mutlicast service uses UDP to send and receive various types of messages.
UDP inherently does not guaranty delivery messages, a message sent can be lost in transit and never be delivered.
The reliable delivery of messages is implemented an abstraction inside `ContinuousMsgSender`.
`ContinuousMsgSender` is a C++ templated class. It internally maintains a list of messages of type `T` and for each
message `M` it maintains a set of recipients `R`. At a interval `t`, it sends out message `M` to all its
recipients in set `R`. It exposes a method `removeRecipient(uint32_t messageId, const std::string &recipient)`
to remove a recipient from the recipients set `R`. A message `M` is a part of the internal list until the recipients set `R`
becomes empty.

The initial interval is 200ms and then it increases exponentially upto a `maxSendingInterval=4000ms` (400ms, 800ms, 1600ms, 3200ms).
Once the `maxSendingInterval` is reached, the interval is reset to 200ms.

Thus a message sent using `ContinuousMsgSender` will be retransmitted at a fixed interval no matter whether
it is received or not at the receiver.

### State Diagram
The state machine of the Multicast Service is as follows:
![State Machine](lab1-state-machine.png)

### Implementing the State Machine

- In order to send a multicast message, a sender `s` creates a `DataMessage` and queues it with an instance of
`ContinuousMsgSender<DataMessage>`. `ContinuousMsgSender<DataMessage>` starts sending the `DataMessage` to all
its recipients.

- On receiving a `DataMessage` at a recipient `r`, `r` add the `DataMessage` to the `HoldBackQueue` along with a
`proposedSeq` and `r` sends `AckMessage` to `s` once.
    The `AckMessage` serves two purposes:
    - It contains the `proposed_seq` for a `DataMessage`
    - It acks as acknowledgement for `s` that `r` received the `DataMessage`

- On receiving the all `AckMessage` at `s`, `s` stores the `proposedSeqId` and removes `r` from the recipients set `R`
inside `ContinuousMsgSender<DataMessage>`, `ContinuousMsgSender<DataMessage>` stops sending `DataMessage` to `r`.
Once `AckMessage` from all recipients is received, `s` creates a `SeqMessage` and queues it with an instance of
`ContinuousMsgSender<SeqMessage>`. `ContinuousMsgSender<SeqMessage>` starts sending the `SeqMessage` to all
its recipients.
    The `SeqMessage` serves two purposes:
    - It contains the `final_seq` for a `DataMessage`
    - It acks as acknowledgement for `r` that `s` received the `DataMessage`

- On receiving a `SeqMessage` at a recipient `r`, `r` marks the `DataMessage` in `HoldBackQueue` as deliverable and
sorts the `HoldBackQueue` and sends `SeqAckMessage` to `s` once. If a `DataMessage` at front of the queue is deliverable,
it delivers it.
    The `SeqAckMessage` serves the following purposes:
    - It acks as acknowledgement for `s` that `r` received the `SeqMessage`

- On receiving a `SeqAckMessage` at `s`, `s` removes `r` from the recipients set `R` inside
`ContinuousMsgSender<SeqMessage>`, `ContinuousMsgSender<SeqMessage>` stops sending `SeqMessage` to `r`.

##### HoldBackQueue
- It internally uses a `deque` to store `PendingMsg`. A pending message consists of the `DataMessage` to be delivered,
the `final_seq`, the `final_seq_proposer` and whether the message is `deliverable` or not.

- No two messages in the `HoldBackQueue` can have same `sender` and same `msg_id`. This invariant helps discard the
duplicate `DataMessage` which are received due to retransmission.

- As soon as a `PendingMsg` is marked deliverable, the `deque` is sorted according to the `final_seq`. If two messages
have same `final_seq`, then the one which is undeliverable is placed ahead of deliverable. If two messages have same
`final_seq` and are undeliverable, then the message with smaller `final_seq_proposer` is placed ahead.

##### How a process delivers its own message?
- The process also sends the multicast `DataMessage` to itself and follows the state machine to deliver its own message.

- This approach helps to keep the design generic and implementation simple. This simplified design helps the
implementation to avoid using unnecessary `if-else`.

##### What happens if a message is dropped or delayed?

*Note:* A `MsgIdentifier` which consists of `sender` and `msg_id` is used to identify duplicate messages.

- `DataMessage`
    - If dropped or delayed, it will be retransmitted by `ContinuousMsgSender<DataMessage>` at `s` until a `AckMessage` is
    received from `r`.
    - Duplicate `DataMessage` received at `r` will be dropped.

- `AckMessage`
    - If dropped or delayed, `s` will retransmit `DataMessage` which will force `r` to send `AckMessage` to `s`. This cycle will
    continue until a `AckMessage` is received at `s`.
    - Duplicate `AckMessage` received at `s` will be dropped.

- `SeqMessage`
    - If dropped or delayed, it will be retransmitted by `ContinuousMsgSender<SeqMessage>` at `s` until a `SeqAckMessage` is
    received from `r`.
    - Duplicate `SeqMessage` will be dropped at `r`

- `SeqAckMessage`
    - If dropped or delayed, `s` will retransmit `SeqMessage` which will force `r` to send `SeqAckMessage` to `s`. This cycle will
    continue until a `SeqAckMessage` is received at `s`.
    - Duplicate `SeqAckMessage` received at `s` will be dropped.


- Snapshot
    - what all is printed, when is printed
- testing suite
    - what happens here
    - delivery message content is checked for all processes, by transitivity

- how a self message is delivered

- the message delay test may take a while to complete since, 50% of the messages are being delayed by 2000ms
- specify why data and seq message are delayed inherently (due to sending interval)

- dropping will show data and seq majorly due to the continuous message based design

- the containers do not exit on their own, but are stopped in the tearDown phase of the test

- the local state of the sender will only contain data for continuous sender

- breaking ties, two senders by induction n senders.