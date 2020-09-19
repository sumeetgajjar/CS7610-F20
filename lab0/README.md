# Steps to run the code

### 1. Building the docker image
Command: `docker build . -t sumeet-g-prj0`

**Note:** I have purposefully tagged the image with my name to prevent accidental running of image with same name for my submission.

### 2. Starting the docker containers
**Note:** In case there are changes made to `hostfile`, make sure to re-build the docker image before starting the containers.

Command: `./start-docker-containers.sh`

The above script does the following:

* Checks if the required docker network bridge exists, if not it creates it.
* starts the docker container for each host present in the hostfile.
    * each container is named using the following format: `sumeet-g-<hostname>`.
    * For each docker container, `stdout` and `stderr` is redirected to a `sumeet-g-<hostname>.log` file for easy access and debugging.

**Note:** On receiving "I am alive" message from all peers, each container outputs "READY" in the log file and exits gracefully.
### Stopping the docker containers
**Note:** This script should only be used to stop all the containers abruptly.

Command: `./stop-docker-containers.sh`

The above script finds and stops all containers with name prefix `sumeet-g*`.