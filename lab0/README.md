## Steps to run the code

### Building the docker image
Command: `docker build . -t sumeet-g-prj0`

**Note:** I have purposefully tagged the image with my name to prevent accidental running of image with same name for my submission.

### Starting the docker containers

Command: `./start-docker-containers.sh`

The above script does the following:

* Checks if the required docker network bridge exists, if not it creates it.
* starts the docker container for each host present in the hostfile.
    * each container is named using following format: `sumeet-g-<hostname>`.
    * For each docker container, `stdout` and `stderr` is redirected to a `sumeet-g-<hostname>.log` file for easy access and debugging.
    
### Stopping the docker containers

Command: `./stop-docker-containers.sh`

The above script finds and stops all containers with name prefix `sumeet-g*`.