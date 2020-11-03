# Membership Service with Failure Detector

## Architecture

### Assumptions
- The system is designed to work with bounded number of peers. This is controlled by the
[MAX_PEERS](src/message.h#L12). <br/>
For the purpose of this lab, this value is set to 10, it can be increased to a valid positive integer value.
Changing this value requires recompilation.

- The system is designed to handle only one event at a time - e.g. one process joining or a process leaving the group.
The system is not designed to handle ”cascading events” - i.e. all joins and leaves finish before another event starts. 
The only exception is for the testcase of the leader failure as described in `test case 4` requirements.

### MembershipService

##### what happens when a leader starts?
- speak about the initial leader and how the state changes
##### what happens when a peer joins?
- talk about initial pending request and it contents
- when is pending request cleared
##### what happens if a peer crashes?
- all the crashes will definitely be detected in the leader but may or may not be detected in the followers 
due to weird timing issue
##### what happens if the Leader crashes?

### FailureDetector
- This service as the name suggests is responsible for detecting a peer crash. It does this by monitoring `HeartBeatMsg`
messages that are sent by peers to each other. 
- During its initialization, the service accepts a `onFailureCallback` of type `std::function<void(PeerId)>`. 
The peer id of the crashed peer is passed as an argument while invoking the callback,
e.g. `onFailureCallback(crashedPeerId)`. This callback is invoked whenever a peer crash is detected. The callback aids
in integrating this service with any service without tight coupling.
- The communication happens using UDP.
- On detecting a peer crash, the service invokes a 
- On `start()`, this class spawns three thread:
    - HeartBeatListener
        - It is responsible for receiving the `HeartBeatMsg` message from peers and update the appropriate data
        structures on receipt.
    - HeartBeatSender
        - It is responsible for sending the `HeartBeatMsg` message to all peers present in the `hostfile`.
        - The `HeartBeatMsg` message will be sent to a peer irrespective of its alive status at a fixed interval defined
        by [HEARTBEAT_INTERVAL_MS](src/failure_detector.h#L13). <br/>
        For the purpose of this lab, this value is set to `1000` ms.
    - HeartBeatMonitor
        - It is responsible for checking if `HeartBeatMsg` message from a peer was received or not. This check happens
        at a fixed interval defined by [HEARTBEAT_INTERVAL_MS](src/failure_detector.h#L13).
        - If `HeartBeatMsg` from a peer is missed for two consecutive intervals, then the peer is declared as dead and
        the corresponding `onFailureCallback` is invoked. 

### Integrating Membership service with Failure Detector
- The `FailureDetector` communicates with the `MembershipService` using the callback Mechanism. This de-couples both
services from each and other. Thus there is no need for any of the services to know about each other and thus can be
plugged in and out as required.
- `MembershipService` exposes a public method `handlePeerFailure(PeerId crashedPeerId)` method which handles the peer
crashes
- `MembershipService::handlePeerFailure` is passed as the `onFailureCallback` callback to `FailureDetector`, it is
invoked when a peer crash is detected.

### Helper Classes
- [PeerInfo](src/utils.h#L29)
    - Contains utility methods to get information about a peer given a `hostname` or a `peerId`.
- [SerDe](src/serde.h)
    - Contains methods to serialize and deserialize all message types.
    - Contains method to get the message type given the raw serialized bytes.
- [Transport](src/network_utils.h)
    - [UDPReceiver](src/network_utils.h#L60)
        - A wrapper class to abstract receiving UDP messages.
    - [UDPSender](src/network_utils.h#L44)
        - A wrapper class to abstract sending UDP messages.
    - [TcpServer](src/network_utils.h#L96)
        - A wrapper class to abstract receiving TCP messages.
    - [TcpClient](src/network_utils.h#L74)
        - A wrapper class to abstract sending TCP messages.

### Libraries
- logging
    - [glog](https://github.com/google/glog) library is used for logging purposes.
- cmd args
    - [gflags](https://github.com/gflags/gflags) library is used for parsing and processing the command line args.
- dependency management
    - C++ does not have a official dependency manager - e.g. pip3 for python3.
    - [conan package manager](https://conan.io/) is used to manage the `glog` and `gflags` dependencies. 
    Conan automates the task of searching and installing the appropriate libraries and saves developer time.

### Implementation Issues
- serializing an deserializing varying size of group members while sending and receiving `NewViewMsg`
    - A `std::vector` can be used to group members of varying sizes however it is not easy to serialize it.
    - A `std::array` on the other hand can be easily serialized and deserialized. The only caveat is that its size 
    cannot be changed after initialization. Because of the earlier assumption of max group size being 10, a fixed size
    `std::array<PeerId, 10> members` is declared in `NewViewMsg`. `NewViewMsg` also contains a field `numberOfMembers`
    which helps in determining the number of members to access in the `members`.

### Testing the MembershipService with FailureDetector

During the development phase, it was important to check whether a process `add` or `del` operation is performed on all
peers and whether they agree on the group members or not. Manual verification helps, but as the number of group members
increases, manual verification becomes tedious and can lead to false positives or true negatives.

To solve this problem, the `MembershipSuite` test suite was developed and can found in `test_membership.py` file. The
test suite contains various test cases to simulate the test cases as mentioned in the lab handout. All the case are
listed under [Test Cases](README.md#Test-Cases) section in README.md.

Each test case invokes the `MulticastSuite.__start_all_containers` method. The `MulticastSuite.__start_all_containers`
method starts docker containers for each host mentioned in the `hostfile`. It waits for View change before starting the
next container, this is to ensure only single event happens at a time. This is done by tailing and waiting for the view
change message in the leader's logs. Once the control is returned from `MulticastSuite.__start_all_containers`, each
test case simulates the corresponding scenarios and asserting on the corresponding conditions. 