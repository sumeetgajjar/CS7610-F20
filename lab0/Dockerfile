FROM ubuntu:18.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends wget build-essential git

RUN wget --no-check-certificate https://github.com/Kitware/CMake/releases/download/v3.17.4/cmake-3.17.4-Linux-x86_64.tar.gz && \
    tar -xzvf cmake-3.17.4-Linux-x86_64.tar.gz

ENV PATH="/cmake-3.17.4-Linux-x86_64/bin:${PATH}"

ARG UDP_PORT=7610
RUN mkdir -p prj0/build prj0/dependencies
COPY *.h *.h.in *.cpp hostfile CMakeLists.txt install_dependencies.sh prj0/

RUN apt-get install -y ca-certificates
RUN /prj0/install_dependencies.sh

WORKDIR prj0/build
RUN cmake .. -DUDP_PORT=$UDP_PORT && make

ENTRYPOINT ["/prj0/build/lab0"]