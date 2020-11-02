# Membership Service with Failure Detector

## Architecture

### Assumptions
- The system is designed to work with bounded number of peers. This is controlled by the
[MAX_PEERS](src/message.h#L12). <br/>
For the purpose of this assignment, this value is set to 10, it can be increased to a valid positive integer value.
Changing this value requires recompilation.

- The system is designed to handle only one event at a time - e.g. one process joining or a process leaving the group.
The system is not designed to handle ”cascading events” - i.e. all joins and leaves finish before another event starts. 
The only exception is for the testcase of the leader failure as described in `test case 4` requirements.




- all the crashes will definitely be detected in the leader but may or may not be detected in the followers 
due to weird timing issue
- talk about initial pending request and it contents
- speak about the initial leader and how the state changes 


### Membership Service

### Failure Detector

### Integrating Membership service with Failure Detector
- The `FailureDetector` communicates with the `MembershipService` using the callback Mechanism. This de-couples both
services from each and other. Thus there is no need for any of the services to know about each other and thus can be
plugged in and out as required.
- `FailureDetector` accepts a `onFailureCallback` callback which is invoked when a peer crash is detected. The peer id
of the crashed peer is passed as an argument while invoking the callback, e.g. `onFailureCallback(crashedPeerId)`.
- `MembershipService` exposes a public method `handlePeerFailure(PeerId crashedPeerId)` method which handles the peer
crashes

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
    - A `std::array` on the other hand can be easily serialized and deserialized. The only caveat is that its size cannot
    be changed after initialization. Because of the earlier assumption of max group size being 10, a fixed size
    `std::array<PeerId, 10> members` is declared in `NewViewMsg`. `NewViewMsg` also contains a field `numberOfMembers`
    which helps in determining the number of members to access in the `members`.