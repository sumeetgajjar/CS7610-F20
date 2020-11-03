# Membership Service with Failure Detector

## Architecture

### Assumptions

- The system is designed to work with bounded number of peers. This is controlled by the
[MAX_PEERS](src/message.h#L12). <br/>
For the purpose of this lab, this value is set to 10, it can be increased to a valid positive integer value.
Changing this value requires recompilation of binaries.

- The system is designed to handle only one event at a time - e.g. one peer joining or a peer leaving the group.
The system is not designed to handle ”cascading events” - i.e. all joins and leaves finish before another event starts.
The only exception is for the testcase of the leader failure as described in `test case 4` requirements.

### MembershipService

- This service is where majority of the action happens.

- It uses TCP for sending and receiving messages between peers.

- ##### what happens when a peer starts?

    - When a peer starts, it tries looking for a leader. It does this by trying to establish a connection on
    `MEMBERSHIP_PORT` with a peer with lowest `PeerId`. If successful that peer is considered as leader and this peer
    becomes a follower.

        ```
        I1103 23:04:00.389497     6 membership.cpp:270] trying hostname: sumeet-g-alpha, peerId: 1 as leader
        I1103 23:04:00.390352     6 membership.cpp:275] found leader, leaderPeerId: 1
        ```

        If not it move-ons to the next peer in the list. Once it has tried all the peers except itself, it must
        become the leader.

        ```
        I1103 23:06:26.763530     6 membership.cpp:270] trying hostname: sumeet-g-beta, peerId: 2 as leader
        E1103 23:06:26.792054     6 network_utils.cpp:291] tcp client failed to connect to host: sumeet-g-beta:10000
        I1103 23:06:26.792263     6 membership.cpp:278] unable to connect to peerId: 2
        I1103 23:06:26.792270     6 membership.cpp:270] trying hostname: sumeet-g-gamma, peerId: 3 as leader
        E1103 23:06:26.844892     6 network_utils.cpp:291] tcp client failed to connect to host: sumeet-g-gamma:10000
        I1103 23:06:26.845032     6 membership.cpp:278] unable to connect to peerId: 3
        I1103 23:06:26.845064     6 membership.cpp:270] trying hostname: sumeet-g-delta, peerId: 4 as leader
        E1103 23:06:26.905181     6 network_utils.cpp:291] tcp client failed to connect to host: sumeet-g-delta:10000
        I1103 23:06:26.905328     6 membership.cpp:278] unable to connect to peerId: 4
        I1103 23:06:26.905360     6 membership.cpp:270] trying hostname: sumeet-g-epsilon, peerId: 5 as leader
        E1103 23:06:26.918989     6 network_utils.cpp:291] tcp client failed to connect to host: sumeet-g-epsilon:10000
        I1103 23:06:26.919117     6 membership.cpp:278] unable to connect to peerId: 5
        I1103 23:06:26.919139     6 membership.cpp:270] trying hostname: sumeet-g-zeta, peerId: 6 as leader
        E1103 23:06:26.931790     6 network_utils.cpp:291] tcp client failed to connect to host: sumeet-g-zeta:10000
        I1103 23:06:26.931890     6 membership.cpp:278] unable to connect to peerId: 6
        I1103 23:06:26.931903     6 membership.cpp:283] no leader found, I am becoming the leader, leaderPeerId: 1
        ```

    - When a peer starts as a leader, it is the only peer in the group, it logs the following:

        ```
        I1103 22:33:16.371945     6 membership.cpp:365] installed view info, viewId: 1, members: {1}
        ```

- ##### How a peer joins the group?

    - A leader maintains a stateful connection with all its followers, the connection can only be lost in-case the
    follower crashes. So if a leader receives a new connection on `MEMBERSHIP_PORT`, it is a sufficient indication that
    it is new peer who is trying to join the group.

        ```
        I1103 22:33:59.460141     6 membership.cpp:324] new peer detected, peerId: 6, hostname: sumeet-g-zeta
        ```

    - When a peers connects to a leader, it sends the `RequestMsg` message to all "existing members" of the group and
    waits for `OkMsg` message from them. Once all `OkMsg` messages are received, the leader sends the `NewViewMsg`
    message to all existing members as well as the new peer.

        ```
        I1103 22:33:59.460193     6 membership.cpp:66] sending RequestMsg: msgType: RequestMsg, requestId: 6, currentViewId: 5, operationType: AddOperation, peerId: 6
        I1103 22:33:59.460665     6 membership.cpp:189] received okMsg: msgType: OkMsg, requestId: 6, currentViewId: 5, from peerId: 2
        I1103 22:33:59.460742     6 membership.cpp:189] received okMsg: msgType: OkMsg, requestId: 6, currentViewId: 5, from peerId: 3
        I1103 22:33:59.461014     6 membership.cpp:189] received okMsg: msgType: OkMsg, requestId: 6, currentViewId: 5, from peerId: 4
        I1103 22:33:59.461069     6 membership.cpp:189] received okMsg: msgType: OkMsg, requestId: 6, currentViewId: 5, from peerId: 5
        I1103 22:33:59.461092     6 membership.cpp:254] added peerId: 6 to the group, GroupSize: 6
        I1103 22:33:59.461135     6 membership.cpp:365] installed view info, viewId: 6, members: {1, 2, 3, 4, 5, 6}
        I1103 22:33:59.461159     6 membership.cpp:108] sending NewViewMsg: msgType: NewViewMsg, newViewId: 6, numberOfMembers: 6, members: {1, 2, 3, 4, 5, 6}
        I1103 22:33:59.461524     6 membership.cpp:117] newViewMsg delivered to all peers
        ```

    - On receiving a `RequestMsg` message from a leader, a peer saves the `RequestMsg` message in
    `RequestMsg pendingRequest;` Once the peer receives the `NewViewMsg` it clears the `pendingRequest` since it now
    completed

        ```
        I1103 22:33:34.954869     7 membership.cpp:157] received requestMsg: msgType: RequestMsg, requestId: 3, currentViewId: 2, operationType: AddOperation, peerId: 3, from leader: 1
        I1103 22:33:34.954887     7 membership.cpp:87] sending OkMsg: msgType: OkMsg, requestId: 3, currentViewId: 2
        I1103 22:33:34.954988     7 membership.cpp:163] received newViewMsg: msgType: NewViewMsg, newViewId: 3, numberOfMembers: 3, members: {1, 2, 3}, from leader: 1
        I1103 22:33:34.955008     7 membership.cpp:365] installed view info, viewId: 3, members: {1, 2, 3}
        ```

- ##### what happens if a peer crashes?

    - A peer crash is detected by the failure detector on a follower it logs the following

      ```
        W1103 23:07:23.034054    14 failure_detector.cpp:78] Peer: 6 is not reachable
      ```
    - When a leader detects that a peer has crashed, it modifies the group membership by deleting the crashed peer from
    the group.

- ##### what happens if the Leader crashes?

    - On detecting a leader crash, all peers determine the leader candidate. A peer who is alive and has the lowest
    `PeerId` in the group becomes the leader. Once the leader candidate is determined, the corresponding peer sends the
    `NewLeaderMsg` message to others and waits for `pendingRequestMsg` message. The new leader now completes the pending
    request if any. While sending the `NewLeaderMsg` message if the new leader is not able to connect to some of the
    peers then it removes them from the group.
        ```
        W1103 23:07:25.034552    14 failure_detector.cpp:78] Peer: 1 is not reachable
        W1103 23:07:25.034685    14 membership.cpp:373] Leader: 1 is not reachable
        I1103 23:07:25.034814     6 membership.cpp:294] new leader candidate peerId: 2
        I1103 23:07:25.034914     6 membership.cpp:296] I am the new leader
        I1103 23:07:25.034946     6 membership.cpp:134] sending newLeaderMsg: msgType: NewLeaderMsg, requestId: 2, currentViewId: 6, operationType: PendingOperation
        E1103 23:07:25.036283     6 network_utils.cpp:291] tcp client failed to connect to host: sumeet-g-gamma:10000
        E1103 23:07:26.058779     6 network_utils.cpp:291] tcp client failed to connect to host: sumeet-g-zeta:10000
        E1103 23:07:27.097441     6 network_utils.cpp:291] tcp client failed to connect to host: sumeet-g-zeta:10000
        E1103 23:07:28.110874     6 network_utils.cpp:291] tcp client failed to connect to host: sumeet-g-zeta:10000
        E1103 23:07:29.123498     6 network_utils.cpp:291] tcp client failed to connect to host: sumeet-g-zeta:10000
        E1103 23:07:30.147599     6 network_utils.cpp:291] tcp client failed to connect to host: sumeet-g-zeta:10000
        E1103 23:07:31.166594     6 network_utils.cpp:291] tcp client failed to connect to host: sumeet-g-zeta:10000
        E1103 23:07:31.166733     6 membership.cpp:146] cannot send newLeaderMsg to peerId: 6
        I1103 23:07:31.166817     6 membership.cpp:226] received pendingRequestMsg: msgType: RequestMsg, requestId: 7, currentViewId: 6, operationType: DelOperation, peerId: 6, from peerId:3
        I1103 23:07:31.166884     6 membership.cpp:226] received pendingRequestMsg: msgType: RequestMsg, requestId: 7, currentViewId: 6, operationType: DelOperation, peerId: 6, from peerId:4
        I1103 23:07:31.166911     6 membership.cpp:226] received pendingRequestMsg: msgType: RequestMsg, requestId: 7, currentViewId: 6, operationType: DelOperation, peerId: 6, from peerId:5
        I1103 23:07:31.166934     6 membership.cpp:304] completing pending request: msgType: RequestMsg, requestId: 7, currentViewId: 6, operationType: DelOperation, peerId: 6
        I1103 23:07:31.166952     6 membership.cpp:245] removed peerId: 6 from the group, GroupSize: 4
        I1103 23:07:31.166978     6 membership.cpp:66] sending RequestMsg: msgType: RequestMsg, requestId: 3, currentViewId: 6, operationType: DelOperation, peerId: 6
        I1103 23:07:31.167294     6 membership.cpp:189] received okMsg: msgType: OkMsg, requestId: 3, currentViewId: 6, from peerId: 3
        I1103 23:07:31.167371     6 membership.cpp:189] received okMsg: msgType: OkMsg, requestId: 3, currentViewId: 6, from peerId: 4
        I1103 23:07:31.167409     6 membership.cpp:189] received okMsg: msgType: OkMsg, requestId: 3, currentViewId: 6, from peerId: 5
        I1103 23:07:31.167456     6 membership.cpp:366] installed view info, viewId: 7, members: {2, 3, 4, 5}
        I1103 23:07:31.167503     6 membership.cpp:108] sending NewViewMsg: msgType: NewViewMsg, newViewId: 7, numberOfMembers: 4, members: {2, 3, 4, 5}
        I1103 23:07:31.167706     6 membership.cpp:117] newViewMsg delivered to all peers
        ```

### FailureDetector

- This service as the name suggests is responsible for detecting a peer crash. It does this by monitoring `HeartBeatMsg`
messages that are sent by peers to each other.

- It maintains a list of `onFailureCallbacks` which are invoked when a peer crash is detected. Each callback is of type
`std::function<void(PeerId)>`.  The peer id of the crashed peer is passed as an argument while invoking the callback,
e.g. `onFailureCallback(crashedPeerId)`. The callback aids in integrating this service with any service without tight
coupling.

- It uses UDP for sending and receiving messages between peers.

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

- `MembershipService::handlePeerFailure` is added to the `onFailureCallbacks` list inside `FailureDetector`, and is
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

    - [gflags](https://github.com/gflags/gflags) library is used for parsing and peering the command line args.

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

- During the development phase, it was important to check whether a peer `add` or `del` operation is performed on all
peers and whether they agree on the group members or not. Manual verification helps, but as the number of group members
increases, manual verification becomes tedious and can lead to false positives or true negatives.

- To solve this problem, the `MembershipSuite` test suite was developed and can found in `test_membership.py` file. The
test suite contains various test cases to simulate the test cases as mentioned in the lab handout. All the case are
listed under [Test Cases](README.md#Test-Cases) section in README.md.

- Each test case invokes the `MulticastSuite.__start_all_containers` method. The `MulticastSuite.__start_all_containers`
method starts docker containers for each host mentioned in the `hostfile`. It waits for View change before starting the
next container, this is to ensure only single event happens at a time. This is done by tailing and waiting for the view
change message in the leader's logs. Once the control is returned from `MulticastSuite.__start_all_containers`, each
test case simulates the corresponding scenarios and asserting on the corresponding conditions.