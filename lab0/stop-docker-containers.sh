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
  docker ps --filter name="sumeet-g*" --format="{{.ID}}" | xargs docker stop
fi
