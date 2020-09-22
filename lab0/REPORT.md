### HeartbeatReceiver
This class is responsible for listening for messages. There are two kind of messages:
1. "I am alive": This is sent by all hosts to all the peers.
2. "Ack": This message is a acknowledgement for the "I am alive" message.

### HeartbeatSender
This class is responsible for sending messages to its peer. It can send the above two kind of messages.

**Note:** `HeartbeatSender` and `HeartbeatReceiver` are running in separate threads.

## Algorithm

1. `HeartbeatReceiver` has a `validSenders` set which initially contains hostnames of all peers.
2. `HeartbeatReceiver` starts listening for messages in a loop in a new thread.
3. `HeartbeatSender` maintains a `aliveMessageReceivers` list which initially contains hostnames of all peers to send "I am alive" message.
4. Once `HeartbeatReceiver` has started listening for messages, `HeartbeatSender` starts sending "I am alive" messages to all peers in a loop at fixed interval.
5. `HeartbeatReceiver`, on receiving "I am alive" message from a peer `p1`
    1. signals `HeartbeatSender` to send "Ack" message to `p1`.
    2. removes `p1` from `validSenders` set.
    3. returns back to listening for new messages.
6. `HeartbeatReceiver`, on receiving "Ack" message from a peer `p1`
    1. signals `HeartbeatSender` to remove `p1` from its `aliveMessageReceivers` list.
    2. returns back to listening for new messages.
7. Once `aliveMessageReceivers` list is empty, `HeartbeatSender` breaks out of its loop and its thread terminates.
8. Once `aliveMessageReceivers` list and `validSenders` set is empty, `HeartbeatReceiver` breaks out of its loop and its thread terminates.
9. On successful termination of sender and receiver threads, the program outputs "READY".

