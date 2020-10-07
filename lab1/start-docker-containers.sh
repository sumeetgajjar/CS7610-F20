#!/usr/bin/env bash
set -e

NETWORK_BRIDGE_NAME="cs7610-bridge"
NETWORK_BRIDGE_EXISTS=$(docker network ls --filter name=${NETWORK_BRIDGE_NAME} --format "{{.Name}}")
if [ -z "${NETWORK_BRIDGE_EXISTS}" ]; then
  echo "docker network bridge does not exists, creating one"
  docker network create --driver bridge cs7610-bridge
else
  echo "docker network bridge: ${NETWORK_BRIDGE_NAME} exists"
fi

for HOST in $(cat hostfile); do
  echo "starting container: ${HOST}"
  docker run --rm -a stdout -a stderr \
    --name "${HOST}" \
    --network cs7610-bridge \
    --hostname "${HOST}" \
    sumeet-g-prj1 --hostfile /lab1/hostfile --msgCount 4 --v 1 >"${HOST}".log 2>&1 &
done
