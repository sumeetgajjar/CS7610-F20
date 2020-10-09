
- retry mechanism
- testing suite
    - what happens here
    - delivery message content is checked for all processes, by transitivity


- dropping will show data and seq majorly due to the continuous message based design
- delaying of messages when enabled is applied to only 50% of the messages
    - compare the logs using thread id to verify that the message is delayed, should be almost equal to delay
    - when a ack msg is delayed
    - when a seqAck msg is delayed
    - when a seq msg is delayed
    - when a data msg is delayed
