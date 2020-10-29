#!/usr/bin/env bash
set -e

RUNNING_CONTAINERS=$(docker ps --filter name="sumeet-g*" --format="{{.Names}}")
if [ -z "${RUNNING_CONTAINERS}" ]; then
  echo "no running containers found"
else
  echo "========================== running containers =========================="
  echo "${RUNNING_CONTAINERS}"
  echo "========================================================================"
  echo "stopping containers"
  echo "========================================================================"
  docker stop ${RUNNING_CONTAINERS}
fi

RUNNING_CONTAINERS=$(docker ps -a --filter name="sumeet-g*" --format="{{.Names}}")
if [ -n "${RUNNING_CONTAINERS}" ]; then
  echo "========================================================================"
  echo "removing containers"
  echo "========================================================================"
  docker rm ${RUNNING_CONTAINERS}
fi
