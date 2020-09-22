FROM ubuntu:18.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    wget \
    netcat \
    iputils-ping \
    build-essential \
    git \
    ca-certificates

RUN wget --no-check-certificate \
    https://github.com/Kitware/CMake/releases/download/v3.17.4/cmake-3.17.4-Linux-x86_64.tar.gz && \
    tar -xzvf cmake-3.17.4-Linux-x86_64.tar.gz

ENV PATH="/cmake-3.17.4-Linux-x86_64/bin:${PATH}"

ARG UDP_PORT=7610
RUN mkdir -p prj0/build prj0/dependencies

COPY install_dependencies.sh prj0/
RUN /prj0/install_dependencies.sh

COPY *.h *.h.in *.cpp hostfile CMakeLists.txt prj0/
WORKDIR prj0/build
RUN cmake .. -DUDP_PORT=$UDP_PORT && make

ENV GLOG_logtostderr=1
ENTRYPOINT ["/prj0/build/lab0"]