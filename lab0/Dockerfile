FROM ubuntu:18.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends wget build-essential

RUN wget --no-check-certificate https://github.com/Kitware/CMake/releases/download/v3.17.4/cmake-3.17.4-Linux-x86_64.tar.gz && \
    tar -xzvf cmake-3.17.4-Linux-x86_64.tar.gz

ENV PATH="/cmake-3.17.4-Linux-x86_64/bin:${PATH}"

RUN mkdir -p prj0/build
COPY *.h *.h.in *.cpp hostfile CMakeLists.txt prj0/

ARG UDP_PORT=7610
WORKDIR prj0/build
RUN cmake .. -DUDP_PORT=$UDP_PORT && make

ENTRYPOINT ./lab0