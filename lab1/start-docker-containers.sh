#!/usr/bin/env bash
set -e

if [ -n "${VERBOSE}" ]; then
  VERBOSE="--v 1"
  echo "verbose mode enabled"
fi

NETWORK_BRIDGE_NAME="cs7610-bridge"
NETWORK_BRIDGE_EXISTS=$(docker network ls --filter name=${NETWORK_BRIDGE_NAME} --format "{{.Name}}")
if [ -z "${NETWORK_BRIDGE_EXISTS}" ]; then
  echo "docker network bridge does not exists, creating one"
  docker network create --driver bridge cs7610-bridge
else
  echo "docker network bridge: ${NETWORK_BRIDGE_NAME} exists"
fi

RUNNING_CONTAINERS=$(docker ps --filter name="sumeet-g*" --format="{{.Names}}")
if [ -z "${RUNNING_CONTAINERS}" ]; then
  echo "no running containers found"
else
  echo "========================== running containers =========================="
  echo "${RUNNING_CONTAINERS}"
  echo "========================================================================"
  echo "stopping containers"
  docker ps --filter name="sumeet-g*" --format="{{.ID}}" | xargs docker stop
fi

for HOST in $(cat hostfile); do
  echo "starting container: ${HOST}"
  docker run --rm -a stdout -a stderr \
    --name "${HOST}" \
    --network cs7610-bridge \
    --hostname "${HOST}" \
    sumeet-g-prj1 --hostfile /lab1/hostfile --msgCount 4 "${VERBOSE}" >"${HOST}".log 2>&1 &
done
