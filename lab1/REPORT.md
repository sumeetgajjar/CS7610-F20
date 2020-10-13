# TODO complete this
- ContinuousMsgSender
- SeqAckMessage
- HoldBackQueue
    - when it is printed, what is printed
- Snapshot
    - what all is printed, when is printed
- testing suite
    - what happens here
    - delivery message content is checked for all processes, by transitivity

- the retry interval is 200ms and then it grows exponentially(400ms, 800ms, 1600ms, 3200ms) with a cutoff at 4000ms. 
On reaching 4000ms it resets back to 200ms. 

- the message delay test may take a while to complete since, 50% of the messages are being delayed by 2000ms
- specify why data and seq message are delayed inherently (due to sending interval)

- dropping will show data and seq majorly due to the continuous message based design
- delaying of messages when enabled is applied to only 50% of the messages
    - compare the logs using thread id to verify that the message is delayed, should be almost equal to delay
    - when a ack msg is delayed
    - when a seqAck msg is delayed
    - when a seq msg is delayed
    - when a data msg is delayed

- the containers do not exit on their own, but are stopped in the tearDown phase of the test

- the local state of the sender will only contain data for continuous sender