# Steps to run the code

### Building the Docker image
Command: `docker build . -t sumeet-g-prj2`

*Note:* The image is purposefully tagged with my name prefix to prevent accidental collision of image names with other
submissions.

### Starting the Docker containers
*Note:*
- In case there are changes made to `hostfile`, please make sure to re-build the docker image before
starting the containers.

- Once started the containers will remain running unless explicitly stopped. The containers will be running even if
there are no messages left for multicast.


#### How to run the testcases
Use `test_membership.py` to run the various test cases (**Recommended and requires python3**) <br/>
It is a python script which uses `unittest` framework to test various multicasting and snapshotting scenarios. <br/>
All the test cases are present under the `MembershipSuite` test suite.
- Before running a test case, `MembershipSuite.setUp()` method is invoked by the unittest framework.
Inside this method all the containers with name prefix `sumeet-g*` are stopped (`docker stop`) and then removed
(`docker rm`).

- After running a test case, `MembershipSuite.tearDown()` method is invoked by the unittest framework.
Inside this method all the containers with name prefix `sumeet-g*` are stopped. The containers are not removed
purposefully so that their logs can be accessed in case of a failure.

##### Test Cases
- membership service test cases
    - `test_case_1`: Adding a peer to the membership list<br/>
    Each peer joins one by one, till all the peers from the hostfile have joined.
    Leader starts first followed by other peers. The script waits for a view change to complete before starting
    the next peer.

    - `test_case_2`: Failure Detector <br/>
    All peers are started one by one. Last peer in the hostfile is crashed and all peers detect its failure.

    - `test_case_3`: Delete a peer from the membership list <br/>
    After all the peers are part of the group, each peer is crashed one by one in the reverse
    order of appearance in the hostfile. The script waits for each view change to complete before crashing
    the next peer. In the end, you only leader is left.

    - `test_case_4`: Leader Failure <br/>
    After all the peers are part of the group, the last peer in the hostfile is crashed. The leader
    detects the failure and sends request message to all peers except the `nextLeader` and dies before accepting any
    `OkMsg`. The new leader starts sends the `NewLeaderMsg` and completes the pending delete request.

Following command should be used to run the a given test case: <br/>
Command: `python3 -m unittest test_membership.MembershipSuite.<test case name> -v` <br/>

The verbosity of the logs can be increased by running the same command with VERBOSE=1 <br/>
Command: `VERBOSE=1 python3 -m unittest test_membership.MembershipSuite.<test case name> -v` <br/>

Logs:
- The application logs are emitted to `stdout` and `stderr`, which can be accessed using `docker logs <hostname>`.
**(Recommended)**

- Along with this, a `logs/<hostname>` folder is created for each host and is mounted to the corresponding docker container.
So along with `docker logs`, logs are also present under `logs/<hostname>/lab2.INFO` file.
This redundancy of logs is to prevent loss of logs in case the docker container is unknowingly removed.

### Stopping the docker containers
Command: `./stop-docker-containers.sh`

The above script finds and stops all containers with name prefix `sumeet-g*`.