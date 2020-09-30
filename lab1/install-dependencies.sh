#!/usr/bin/env bash

set -e
DEPENDENCIES="dependenciesCMake"
if [ -d ${DEPENDENCIES} ]; then
  echo "Removing dependencies dir: '${DEPENDENCIES}'"
  rm -rf ${DEPENDENCIES}
fi

echo "Creating dependencies dir: '${DEPENDENCIES}'"
mkdir ${DEPENDENCIES}

conan install . \
    --generator cmake \
    --install-folder=${DEPENDENCIES} \
    -r conan-center \
    -s os=Linux \
    -s os_build=Linux \
    -s arch=x86_64 \
    -s arch_build=x86_64 \
    -s compiler=gcc \
    -s compiler.version=7 \
    -s compiler.libcxx=libstdc++11 \
    -s build_type=Release \
    -s cppstd=17