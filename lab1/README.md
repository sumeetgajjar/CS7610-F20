# Steps to run the code

### Building the Docker image
Command: `docker build . -t sumeet-g-prj1`

*Note:* The image is purposefully tagged with my name prefix to prevent
accidental collision of image names with others submission.

### Starting the Docker containers
*Note:*
- In case there are changes made to `hostfile`, please make sure to re-build the docker image before
starting the containers.

- Once started the containers will remain running unless explicitly stopped. The containers will be running even if
there are no messages left for multicast.


There are 2 ways to run the containers.
1. Use `test_multicast.py` to run the various test cases (**Recommended and requires python3**) <br/>
It is a python script which uses `unittest` python framework to test various multicasting and snapshotting scenarios. <br/>
All the test cases are present under the `MulticastSuite`.
    - Before running a test case, `MulticastSuite.setUp()` method is invoked by the unittest framework.
    Inside this method all the containers with name prefix `sumeet-g*` are stopped and then removed.

    - After running a test case, `MulticastSuite.tearDown()` method is invoked by the unittest framework.
    Inside this method all the containers with name prefix `sumeet-g*` are stopped. The containers are not removed
    purposefully so that their logs can be accessed in case of a failure.

    - **AssertCondition:** Once all messages are delivered to the processes, all the tests assert on the delivery order of messages for each process.
    The delivery order should be the same for all the processes, if not the assertion throws an exception and hence the test case fails.

    ##### Test Cases
    - mutlicast test cases
        - `test_one_sender`: runs the containers with first host in the `hostfile` as sender

        - `test_two_senders`: runs the containers with first two hosts in the `hostfile` as senders

        - `test_all_senders`: runs the containers with all hosts in the `hostfile` as senders

        - `test_drop_half_messages`: runs the containers with `--dropRate 0.5` and the first host in the `hostfile` as sender

        - `test_drop_majority_messages`: runs the containers with `--dropRate 0.75` and the first host in the `hostfile` as sender <br/>
        *Note:* This test case might take a while to complete since 75% of the messages are being dropped.

        - `test_delay_messages`: runs the containers with `--delay 2000` and the first host in the `hostfile` as sender <br/>

        - `test_drop_delay_messages`: runs the containers with `--dropRate 0.25`, `--delay 2000` and the first host in the `hostfile` as sender <br/>
        *Note:* This test case might take a while to complete since 25% of the messages are being dropped and 50% of the messages are being delayed by 2000ms.

    - mutlicast with snapshot test cases

        - `test_snapshot_with_one_sender`: runs the containers with first host in the `hostfile` as sender.
        The snapshot initiator is the first host in the `hostfile` and is initiated after three sending multicast messages i.e. `--initiateSnapshotCount 3`.

        - `test_snapshot_with_two_senders`: runs the containers with first two host in the `hostfile` as senders.
        The snapshot initiator is the first host in the `hostfile` and is initiated after three sending multicast messages i.e. `--initiateSnapshotCount 3`.

        - `test_snapshot_with_all_senders`: runs the containers with all hosts in the `hostfile` as senders.
        The snapshot initiator is the first host in the `hostfile` and is initiated after three sending multicast messages i.e. `--initiateSnapshotCount 3`.

    Following command should be used to run the a given test case: <br/>
    Command: `python3 -m unittest test_multicast.MulticastSuite.<test case name> -v` <br/>

    The verbosity of the logs can be increased by running the same command with VERBOSE=1 <br/>
    Command: `VERBOSE=1 python3 -m unittest test_multicast.MulticastSuite.<test case name> -v` <br/>

    Logs:
    - The application logs are emitted to `stdout` and `stderr`, which can be accessed using `docker logs <hostname>`.

    - Along with this, a `logs/<hostname>` folder is created for each host and is mounted to the corresponding docker container.
    So along with `docker logs`, logs are also present under `logs/<hostname>/lab1.INFO` file.
    This redundancy of logs is to prevent loss of logs in cases where the docker container is removed.

2. Manually start the container for each `hostname` in the `hostfile`. <br/>
Command: `HOST='<hostname>'; docker run --name "${HOST}" --network cs7610-bridge --hostname "${HOST}" sumeet-g-prj1 --v 0 --hostfile /hostfile  --senders "sumeet-g-alpha" --msgCount 4 --dropRate 0.0 --delay 0 --initiateSnapshotCount 0`
    - --v: Controls the verbosity of the logs. Setting it to 1 will increase the amount of logs.
    Increasing the verbosity is only useful during the debugging phase but not in general.
    It is recommended to not set this flag to 1 unless required.

    - --hostfile: The path of the hostfile inside the docker container

    - --senders: Comma separated list of senders. E.g. `--senders "<host1>,<host2>,<host3>"`.
    The list should only contain the hosts from the `hostfile`.

    - --msgCount: Indicates the number of messages each host in the `--senders` list needs to multicast. <br/>
    *Note:* If `--msgCount` is set to a value and `--senders` is empty, no messages will be multicasted.

    - --dropRate: the ratio of messages to drop. Acceptable range is between 0 and 1.

    - --delay: the amount of network delay in milliseconds.

    - --initiateSnapshotCount: the number of messages after which the process will start the snapshot. <br/>
    **WARNING, WARNING, WARNING**: This flag should only be set for a single docker container.
    If this flag is set for multiple docker containers, it will result in undefined behavior.

    Logs:
    - The application logs are emitted to `stdout` and `stderr`, which can be accessed using `docker logs <hostname>`.
### Stopping the docker containers
Command: `./stop-docker-containers.sh`

The above script finds and stops all containers with name prefix `sumeet-g*`.